#include "Evaluation.h"

#include <stdexcept>
#include <limits>
#include <future>

#include <iostream>
#include "IO.h"

#include "Board.h"
#include "MoveGenerator.h"
#include "ThreadPool.h"
#include "MovePatterns.h"

#define MOVE_ORDERING
#define ITERATIVE_DEEPENING

namespace GGChess
{
	static std::array<uint8_t, TABLE_SIZE> transposTable;

	const Value
		MAX_VALUE = std::numeric_limits<Value>::max(),
		MIN_VALUE = std::numeric_limits<Value>::min() + 1;

	const int phaseInc[7] = { 0, 0, 4, 1, 1, 2, 0 };

	RootMove::RootMove(const Move& move) :
		myMove(move),
		score(MIN_VALUE),
		prevScore(MIN_VALUE)
	{}

	bool RootMove::operator < (const RootMove& other) const {
		return score != other.score ?
			score > other.score :
			prevScore > other.prevScore;
	}

	static inline int Flip(int square) {
		return square ^ 56;
	}

	Value EvaluatePos(Board& board, const PosInfo& pos)
	{
		if (transposTable.at(board.GetPosKey() % TABLE_SIZE))
			return 0;

		Value mgScore = 0, egScore = 0;
		Value materialScore = 0;
		int phase = 0;

		for (int i = 0; i < 64; i++) {
			Piece p = board[(Square)i];
			PieceType pt = pieceof(p);

			if (p == Piece::Empty)
				continue;

			Side ps = sideof(p);
			Value persp = ps == board.GetTurn() ? 1 : -1;

			//game phase
			phase += phaseInc[(int)pt];

			// piece value sum
			Value pieceValue = valueof(pt);
			materialScore += pieceValue * persp;

			// piece square table
			int square = ps == Side::White ? Flip(i) : i;
			mgScore += PSTables::middlegame[(int)pt][square] * persp;
			egScore += PSTables::endgame[(int)pt][square] * persp;
		}

		// phase blend
		phase = std::min(phase, 24);
		Value phaseScore = (mgScore * phase + egScore * (24 - phase)) / 24;

		return materialScore + phaseScore;
	}

	static void OrderMoves(Board& board, MoveList& moves)
	{
		if (moves.size() < 2)
			return;

		std::vector<Value> scores(moves.size());

		for (size_t i = 0; i < moves.size(); i++) {
			scores[i] = 0;
			const Move& move = moves[i];

			if (move.captured != Piece::Empty) {
				scores[i] +=
					10 * valueof(move.captured) -
					valueof(board[move.origin]);
			}

			if (move.flags & Move::Flags::Promotion) {
				switch (move.flags) {
				case Move::Flags::PromoteQ: scores[i] += valueof(PieceType::Queen); break;
				case Move::Flags::PromoteR: scores[i] += valueof(PieceType::Rook); break;
				case Move::Flags::PromoteN: scores[i] += valueof(PieceType::Knight); break;
				case Move::Flags::PromoteB: scores[i] += valueof(PieceType::Bishop); break;
				}
			}
		}

		for (size_t i = 0; i < moves.size() - 1; i++) {
			for (size_t j = i + 1; j > 0; j--) {
				size_t swapIndex = j - 1;
				if (scores[swapIndex] < scores[j]) {
					Move mtemp = moves[j];
					moves[j] = moves[swapIndex];
					moves[swapIndex] = mtemp;

					Value stemp = scores[j];
					scores[j] = scores[swapIndex];
					scores[swapIndex] = stemp;
				}
			}
		}
	}

	bool BadCapture(Board& board, Move& move)
	{
		Piece moving = board[move.origin];
		PieceType
			mt = pieceof(moving),
			ct = pieceof(move.captured);

		if (pieceof(moving) == PieceType::Pawn)
			return false;

		if (valueof(mt) - 50 <= valueof(ct))
			return false;

		bool defendedByPawn = false;
		PawnPattern(move.target, sideof(moving), [&](Square target, bool attack) {
			Piece tp = board[target];
			if (pieceof(tp) == PieceType::Pawn && sideof(tp) != sideof(moving))
				defendedByPawn = true;
			return true;
			}, true);

		if (defendedByPawn && valueof(ct) + 200 < valueof(mt))
			return true;

		if (valueof(ct) + 500 < valueof(mt)) {
			bool isAttacked = false;

			KnightPattern(move.target, [&](Square target) {
				if (pieceof(board[target]) == PieceType::Knight && sideof(board[target]) == sideof(move.captured)) {
					isAttacked = true;
					return false;
				}
				return true;
				});

			SlidingPiecePattern(move.target, PieceType::Bishop, [&](Square target, int rayIdx) {
				Piece targetPiece = board[target];

				if (isAttacked)
					return false;

				if (targetPiece != Piece::Empty) {
					if (sideof(targetPiece) == sideof(move.captured) && pieceof(targetPiece) == PieceType::Bishop) {
						isAttacked = true;
					}
					return false;
				}
				return true;
				});

			if (isAttacked)
				return true;
		}

		return false;
	}

	static Value QuiesceSearch(Board& board, const PosInfo& info, Value alpha, Value beta)
	{
		Value eval = EvaluatePos(board, info);
		Value standPat = eval;

		if (eval >= beta)
			return beta;

		if (alpha < eval)
			alpha = eval;

		MoveList moves;
		GetAllCaptures(board, info, moves);
#ifdef MOVE_ORDERING
		OrderMoves(board, moves);
#endif

		for (Move& move : moves) {
			board.PlayMove(move);
			eval = -QuiesceSearch(board, board.GetPosInfo(), -beta, -alpha);
			board.UnplayMove();

			if (BadCapture(board, move) &&
				pieceof(move.captured) != PieceType::Pawn && // TODO can simplify
				!(move.flags & Move::Flags::Promotion))
				continue;

			if (eval > alpha) {
				if (eval >= beta)
					return beta;
				alpha = eval;
			}
		}
		return alpha;
	}

	static Value EvaluationHelper(Board& board, const PosInfo& info, int depth, Value alpha, Value beta)
	{
		if (depth <= 0)
			return EvaluatePos(board, info);

		MoveList moves;
		GetAllMoves(board, info, moves);
#ifdef MOVE_ORDERING
		OrderMoves(board, moves);
#endif

		if (moves.size() == 0) {
			if (info.check)
				return std::numeric_limits<Value>::min() + 1;
			return 0;
		}

		for (const Move& move : moves) {
			board.PlayMove(move);
			Value eval = -EvaluationHelper(board, board.GetPosInfo(), depth - 1, -beta, -alpha);
			board.UnplayMove();

			if (eval >= beta)
				return beta;
			if (eval > alpha)
				alpha = eval;
		}
		return alpha;
	}

	Value Evaluate(Board& board, const PosInfo& info, int depth) {
		transposTable.at(board.GetPosKey() % TABLE_SIZE)++;

		return EvaluationHelper(
			board, info, depth, MIN_VALUE, MAX_VALUE
		);
	}

	struct EvalThread
	{
		Board board;
		int depth;

		EvalThread(Board& board, int depth) :
			board(board), depth(depth)
		{}

		Value operator() () {
			return Evaluate(board, board.GetPosInfo(), depth);
		}
	};

	Move FindBestMove(ThreadPool& pool, Board& board, int depth) // TODO size_t depth
	{
		if (depth < 1)
			throw std::invalid_argument("depth must be minimum 1");

		const PosInfo info = board.GetPosInfo();
		MoveList moves;
		GetAllMoves(board, info, moves);
#ifdef MOVE_ORDERING
		OrderMoves(board, moves);
#endif
		Value bestValue = MIN_VALUE;
		Move bestMove;

#ifdef ITERATIVE_DEEPENING

		std::vector<RootMove> rootMoves;
		rootMoves.reserve(moves.size());
		for (const Move& move : moves)
			rootMoves.push_back(RootMove(move));

		for (size_t currDepth = 1; currDepth <= depth; currDepth++) {
			Value alpha = MIN_VALUE, beta = MAX_VALUE;

			for (RootMove& rootMove : rootMoves) {
				rootMove.prevScore = rootMove.score;

				board.PlayMove(rootMove.myMove);
				rootMove.score = -EvaluationHelper(board, board.GetPosInfo(), currDepth, -beta, -alpha);
				board.UnplayMove();

				alpha = std::max(rootMove.score, alpha);
			}
			std::stable_sort(rootMoves.begin(), rootMoves.end());
			std::cout << "info depth " << currDepth << " move " << std::toString(rootMoves[0].myMove) << std::endl;

			bestMove = rootMoves[0].myMove;
		}
#else
		std::vector<std::shared_future<Value>> futures;
		futures.reserve(moves.size());

		for (const Move& move : moves) {
			board.PlayMove(move);
			/*
			futures.push_back(
				pool.submit<EvalThread, Value>(EvalThread(board, depth - 1))
			);*/
			
			
			Value res = -Evaluate(board, info, depth - 1);

			if (bestValue < res) {
				bestValue = res;
				bestMove = move;
			}
			
			board.UnplayMove();
		}

		/*
		for (size_t i = 0; i < moves.size(); i++) {
			futures[i].wait();
			Value res = -futures[i].get();
			//std::cout << res << std::endl;

			if (bestValue < res) {
				bestValue = res;
				bestMove = moves[i];
			}
		}*/
#endif

		return bestMove;
	}
}

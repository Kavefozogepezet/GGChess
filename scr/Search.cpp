#include "Search.h"

#include <stdexcept>
#include <limits>

#include <iostream>
#include "IO.h"

#include "Board.h"
#include "MoveGenerator.h"
#include "ThreadPool.h"
#include "MovePatterns.h"
#include "TransposTable.h"
#include "UCIHandler.h"

namespace GGChess
{

	void SearchData::reset() {
		nodes = 0;
		qnodes = 0;
		aspf = 0;
		start = clock::now();
	}

	int64_t SearchData::elapsed() {
		clock::duration e = clock::now() - start;
		return std::chrono::duration_cast<std::chrono::milliseconds>(e).count();
	}

	SearchData sdata;


	static std::array<uint8_t, TABLE_SIZE> prevPosTable; // TODO Rewrite this mess

	const Value
		MAX_VALUE = std::numeric_limits<Value>::max(),
		MIN_VALUE = std::numeric_limits<Value>::min() + 1;

	const int phaseInc[7] = { 0, 0, 4, 1, 1, 2, 0 };

	RootMove::RootMove(const Move& move, Value score) :
		myMove(move),
		score(score)
	{}

	bool RootMove::operator < (const RootMove& other) const {
		return score > other.score;
	}

	static inline int Flip(int square) {
		return square ^ 56;
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
		sdata.nodes++;
		sdata.qnodes++;
		
		Value eval = Evaluate(board, info);
		Value standPat = eval;

		if (eval >= beta)
			return beta;

		if (alpha < eval)
			alpha = eval;

		MoveList moves;
		GetAllCaptures(board, info, moves);
		OrderMoves(board, moves);

		for (Move& move : moves) {
			board.PlayMove(move);
			eval = -QuiesceSearch(board, board.Info(), -beta, -alpha);
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

	static Value SearchHelper(Board& board, const PosInfo& info, int depth, Value alpha, Value beta)
	{
		tpostable.prefetch(board.Key());

		prevPosTable.at(board.Key() % TABLE_SIZE)++; // TODO better repetition test

		if (info.check) // Do not evaluate when in check to prevent false result
			depth++;

		if (depth <= 0)
			return QuiesceSearch(board, info, alpha, beta); // search until no capture
			//return EvaluatePos(board, info);

		sdata.nodes++;

		TTEntry ttentry;
		if (tpostable.probe(board.Key(), depth, alpha, beta, ttentry))
			return ttentry.eval; // TODO when pv node

		MoveList moves;
		GetAllMoves(board, info, moves);
		OrderMoves(board, moves);

		if (moves.size() == 0) {
			if (info.check)
				return std::numeric_limits<Value>::min() + 1;
			return 0;
		}

		TTFlag flag = TTFlag::Alpha;
		Move bestmove = moves[0];

		for (const Move& move : moves) {
			board.PlayMove(move);
			Value eval = -SearchHelper(board, board.Info(), depth - 1, -beta, -alpha);
			board.UnplayMove();

			if (eval > alpha) {
				bestmove = move;

				if (eval >= beta) {
					flag = TTFlag::Beta;
					alpha = beta;
					break;
				}
				flag = TTFlag::Exact;
				alpha = eval;
			}
		}

		tpostable.save(board.Key(), depth, alpha, flag, bestmove);
		return alpha;
	}

	RootMove SearchRoot(Board& board, size_t depth, Value alpha, Value beta)
	{
		PosInfo info = board.Info();
		RootMove best;

		if (info.check)
			++depth; // extend search to avoid evaluating position when in check

		MoveList moves;
		GetAllMoves(board, board.Info(), moves);

		if (moves.size() == 1)
			return RootMove(moves[0]);

		OrderMoves(board, moves); // TODO relocate when pv ordering is done

		int i = 0;

		for (const Move& move : moves) {
			board.PlayMove(move);
			Value eval = -SearchHelper(board, board.Info(), depth - 1, -beta, -alpha);
			board.UnplayMove();

			if (eval > alpha) {
				best = RootMove(move, eval);
				if (eval > beta) {
					tpostable.save(board.Key(), depth, eval, TTFlag::Beta, move);
					return best;
				}
			}
			alpha = eval;
			tpostable.save(board.Key(), depth, eval, TTFlag::Alpha, move);
		}
		tpostable.save(board.Key(), depth, alpha, TTFlag::Exact, best.myMove);
		return best;
	}

	Move Search(Board& board, size_t depth) // TODO size_t depth
	{
		sdata.reset();

		RootMove move = SearchRoot(board, 1, MIN_VALUE, MAX_VALUE);
		printSearchData(1, sdata, move);

		for (size_t curr_depth = 2; curr_depth <= depth; curr_depth++)
		{
			Value delta = 50;

			//while (true) {
				Value
					alpha = move.score - delta,
					beta = move.score + delta;

				move = SearchRoot(board, curr_depth, alpha, beta); // search with aspiration window
				if (move.score <= alpha || move.score >= beta) {
					//delta += delta / 4;
					move = SearchRoot(board, curr_depth, MIN_VALUE, MAX_VALUE);
					sdata.aspf++;
				}
				/*else {
					break;
				}*/
			//}

			printSearchData(curr_depth, sdata, move);
		}
		return move.myMove;

	}
}

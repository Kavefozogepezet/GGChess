#include "Search.h"

#include <stdexcept>
#include <limits>
#include <algorithm>

#include <iostream>
#include "IO.h"

#include "Board.h"
#include "MoveGenerator.h"
#include "ThreadPool.h"
#include "MovePatterns.h"
#include "TransposTable.h"
#include "InputHandler.h"

namespace GGChess
{

	void SearchData::reset() {
		nodes = 0;
		qnodes = 0;
		aspf = 0;
		timer.reset();
	}

	void SearchData::allocTime(Limits limits, Board& board)
	{
		int32_t avglen = 40; // average length of a chess match
		int32_t avgrest = std::max(avglen - board.Ply() / 2, 5); // estimate how many moves are to be played

		movetime = 16000; // default 8000 ms == 8 sec

		if (board.Turn() == Side::White && limits.wtime) {
			movetime = limits.wtime / avgrest;
			UCI_INFO << "time: " << limits.wtime << " avgrest: " << avgrest << " allocated: " << movetime << " ms" << std::endl;
		}
		else if (limits.btime) {
			movetime = limits.btime / avgrest;
		}
	}

	bool SearchData::timeout() {
		return movetime < timer.elapsed();
	}

	SearchData sdata;

	static std::array<uint8_t, TABLE_SIZE> prevPosTable; // TODO Rewrite this mess

	const Value
		MAX_VALUE = std::numeric_limits<Value>::max() / 2,
		MIN_VALUE = (std::numeric_limits<Value>::min() + 1) / 2;

	const int phaseInc[7] = { 0, 0, 4, 1, 1, 2, 0 };

	RootMove::RootMove() :
		myMove(Square(100), Square(100)),
		score(MIN_VALUE)
	{}

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

	size_t quiesce_count = 0;

	static Value QuiesceSearch(Board& board, const PosInfo& info, Value alpha, Value beta)
	{
		if (sdata.timeout())
			return 0; // abort search

		sdata.nodes++;
		sdata.qnodes++;
		quiesce_count++;

		printSearchData(sdata);
		
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
			if (standPat + valueof(pieceof(move.captured)) + 200 < alpha &&
				!(move.flags & Move::Flags::Promotion)) // TODO endgame material check
				continue;

			if (BadCapture(board, move) &&
				pieceof(move.captured) != PieceType::Pawn && // TODO can simplify
				!(move.flags & Move::Flags::Promotion))
				continue;

			board.PlayMove(move);
			eval = -QuiesceSearch(board, board.Info(), -beta, -alpha);
			board.UnplayMove();

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
		if (sdata.timeout())
			return 0; // abort search

		tpostable.prefetch(board.Key());

		prevPosTable.at(board.Key() % TABLE_SIZE)++; // TODO better repetition test

		if (info.check && depth <= 0) // Do not evaluate when in check to prevent false result
			depth++;

		printSearchData(sdata);

		if (depth <= 0) {
			return QuiesceSearch(board, info, alpha, beta); // search until no capture
			//quiesce_count = 0;
		}

		sdata.nodes++;

		TTEntry ttentry;
		if (tpostable.probe(board.Key(), depth, alpha, beta, ttentry))
			return ttentry.eval; // TODO when pv node

		MoveList moves;
		GetAllMoves(board, info, moves);

		if (moves.size() == 0) {
			if (info.check)
				return MIN_VALUE + 1;
			return 0;
		}

		OrderMoves(board, moves);

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

	void PickBest(RootList& roots, size_t current) {
		size_t bestIdx = current;
		for (size_t i = current + 1; i < roots.size(); i++) {
			if (roots[bestIdx].score < roots[i].score) {
				bestIdx = i;
			}
		}

		if (current == bestIdx)
			return;

		RootMove root = roots[current];
		roots[current] = roots[bestIdx];
		roots[bestIdx] = root;
	}

	RootMove SearchRoot(Board& board, PosInfo& info, RootList& moves, size_t depth, Value alpha, Value beta)
	{
		RootMove best = moves[0];

		if (info.check)
			++depth; // extend search to avoid evaluating position when in check

		if (moves.size() == 1)
			return best;

		for (size_t i = 0; i < moves.size(); i++) {
			PickBest(moves, i); // TODO Test this
			Move& move = moves[i].myMove;

			board.PlayMove(move);
			Value eval = -SearchHelper(board, board.Info(), depth - 1, -beta, -alpha);
			moves[i].score = eval;
			board.UnplayMove();

			if (sdata.timeout()) // Search returned is invalid, return current best
				return best;

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

	Move Search(Board& board, const Limits& limits) // TODO size_t depth
	{
		sdata.reset();
		sdata.allocTime(limits, board);
		sdata.side = board.Turn();

		PosInfo info = board.Info();

		MoveList moves;
		GetAllMoves(board, board.Info(), moves);
		RootList roots;

		for (Move& move : moves) {
			roots.push_back(move);
		}

		sdata.best = SearchRoot(board, info, roots, 1, MIN_VALUE, MAX_VALUE);
		sdata.depth++;
		printSearchData(sdata);

		for (sdata.depth = 2; !sdata.timeout(); sdata.depth++)
		{
			//sdata.best = SearchRoot(board, sdata.depth, MIN_VALUE, MAX_VALUE);
			
			Value bounds[4][2] = {
				{ sdata.best.score - 10, sdata.best.score + 10 },
				{ sdata.best.score - 25, sdata.best.score + 25 },
				{ sdata.best.score - 50, sdata.best.score + 50 },
				{ MIN_VALUE, MAX_VALUE }
			};

			RootMove tempbest;
			for (size_t i = 0; i < 4; i++) {
				tempbest = SearchRoot(board, info, roots, sdata.depth, bounds[i][0], bounds[i][1]); // search with aspiration window
				if (tempbest.score <= bounds[i][0] || tempbest.score >= bounds[i][1])
					sdata.aspf++;
			}
			sdata.best = tempbest;

			printSearchData(sdata, true);
		}
		return sdata.best.myMove;
	}
}

#pragma once

#include "BasicTypes.h"

#include <array>

namespace GGChess
{
	class Board;
	struct PosInfo;
	class ThreadPool;

	extern const Value MAX_VALUE, MIN_VALUE;

	extern const int phaseInc[7];

	namespace PSTables
	{
		extern const Value
			mgPawn[BOARD_SQUARE_COUNT], mgKnight[BOARD_SQUARE_COUNT], mgBishop[BOARD_SQUARE_COUNT],
			mgRook[BOARD_SQUARE_COUNT], mgQueen[BOARD_SQUARE_COUNT], mgKing[BOARD_SQUARE_COUNT],

			egPawn[BOARD_SQUARE_COUNT], egKnight[BOARD_SQUARE_COUNT], egBishop[BOARD_SQUARE_COUNT],
			egRook[BOARD_SQUARE_COUNT], egQueen[BOARD_SQUARE_COUNT], egKing[BOARD_SQUARE_COUNT];

		extern const Value* middlegame[PIECE_COUNT], *endgame[PIECE_COUNT];
	}

	struct RootMove
	{
		Move myMove;
		Value score;
		Value prevScore;

		RootMove(const Move& move);

		bool operator < (const RootMove& other) const;
	};

	Value EvaluatePos(Board& board, const PosInfo& info);

	Value Evaluate(Board& board, const PosInfo& info, int depth);

	Move FindBestMove(ThreadPool& pool, Board& board, int depth);

	bool BadCapture(Board& board, Move& move);
}

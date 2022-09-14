#pragma once

#include "BasicTypes.h"

#include <array>
#include <chrono>

#include "FastArray.h"
#include "BasicTypes.h"

namespace GGChess
{
	class Board;
	struct PosInfo;

	extern const Value MAX_VALUE, MIN_VALUE;

	struct RootMove
	{
		Move myMove;
		Value score;

		RootMove();
		RootMove(const Move& move, Value score = MIN_VALUE);

		bool operator < (const RootMove& other) const;
	};

	typedef FastArray<RootMove, MAX_MOVES> RootList;

	struct SearchData {
		using clock = typename std::chrono::steady_clock;

		uint64_t nodes;
		uint64_t qnodes;
		uint64_t aspf;

		Timer timer;
		uint64_t movetime;

		Side side;
		size_t depth;
		RootMove best;

		void reset();
		void allocTime(Limits limits, Board& board);
		bool timeout();
	};

	extern SearchData sdata;

	extern const int phaseInc[7];

	namespace PSTables
	{
		extern const Value
			mgPawn[BOARD_SQUARE_COUNT], mgKnight[BOARD_SQUARE_COUNT], mgBishop[BOARD_SQUARE_COUNT],
			mgRook[BOARD_SQUARE_COUNT], mgQueen[BOARD_SQUARE_COUNT], mgKing[BOARD_SQUARE_COUNT],

			egPawn[BOARD_SQUARE_COUNT], egKnight[BOARD_SQUARE_COUNT], egBishop[BOARD_SQUARE_COUNT],
			egRook[BOARD_SQUARE_COUNT], egQueen[BOARD_SQUARE_COUNT], egKing[BOARD_SQUARE_COUNT];

		extern const Value* middlegame[PIECE_COUNT], *endgame[PIECE_COUNT];

		extern const Value
			passedPawn[8], // by rank
			weakPawn[8]; // by file

		extern const Value kingSafetyTable[100];
	}

	Value Evaluate(Board& board, const PosInfo& info);

	Move Search(Board& board, const Limits& limits);

	bool BadCapture(Board& board, Move& move);
}

#pragma once

#include <array>
#include <list>
#include <deque>
#include <exception>
#include <stdexcept>

#include "BasicTypes.h"
#include "FastArray.h"

namespace GGChess
{
	class Board;
	struct PosInfo;

	class invalid_board :
		std::exception
	{
		using std::exception::exception;
	};

	typedef FastArray<Move, MAX_MOVES> MoveList;

	void GetAllMoves(Board& board, const PosInfo& info, MoveList& moves);

	void GetAllCaptures(Board& board, const PosInfo& info, MoveList& moves);

	void GetMoves(Board& board, const PosInfo& info, Square square, MoveList& moves);

	bool IsSquareAttacked(Board& board, const PosInfo& info, Square square, Side attacker);
}

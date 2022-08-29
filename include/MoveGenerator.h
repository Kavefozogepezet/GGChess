#pragma once

#include <array>
#include <list>
#include <deque>
#include <exception>
#include <stdexcept>

#include "BasicTypes.h"

namespace GGChess
{
	class Board;
	struct PosInfo;

	class invalid_board :
		std::exception
	{
		using std::exception::exception;
	};

	class MoveList
	{
	public:
		MoveList();

		size_t size() const;
		Move* begin();
		Move* end();

		void push_back(const Move& move);

		Move& operator [] (size_t idx);
	private:
		Move moveArray[256];
		Move* last;
		size_t count;
	};

	void GetAllMoves(Board& board, const PosInfo& info, MoveList& moves);

	void GetMoves(Board& board, const PosInfo& info, Square square, MoveList& moves);

	bool IsSquareAttacked(Board& board, const PosInfo& info, Square square, Side attacker);
}

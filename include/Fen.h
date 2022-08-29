#pragma once

#include <string>

#include "BasicTypes.h"

namespace GGChess
{
	class Board;

	class Fen
	{
	public:
		Fen() = delete;

		static std::string Get(const Board& board);
		static void Set(Board& board, std::istream& stream);
		static void Set(Board& board, const std::string& fen);
	};
}


#pragma once

#include <iostream>
#include <string>

#include "Basictypes.h"

namespace std {
	std::string toString(GGChess::Square square);
	std::string toString(GGChess::Piece piece);
	std::string toString(const GGChess::Move& move);
}

namespace GGChess
{
	class Board;

	char piece_to_char(Piece piece);
	Piece char_to_piece(char c);

	std::ostream& operator << (std::ostream& stream, const Board& board);
	std::ostream& operator << (std::ostream& stream, const Move& move);
	std::ostream& operator << (std::ostream& stream, const BitBoard& board);

	std::istream& operator >> (std::istream& stream, Square& square);
}

#pragma once

#include <iostream>
#include <string>

#include "Basictypes.h"

namespace std {
	std::string to_string(GGChess::Square square);
	std::string to_string(GGChess::Piece piece);
	std::string to_string(const GGChess::Move& move);
	std::string to_string(GGChess::Side side);
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

	class styledout
	{
	public:
		static const styledout basic;

		static std::ostream& writeWith(const styledout& style);

		static std::ostream& endl(std::ostream& stream) {
			stream << styledout::basic;
			stream << std::endl;
			return stream;
		}

		static std::ostream& nostyle(std::ostream& stream) {
			stream << styledout::basic;
			return stream;
		}
	public:
		enum ColorCode {
			BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, LIGHT_GRAY,
			GRAY = 90, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW, BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, WHITE
		};

		typedef unsigned char color8b;
	public:
		styledout();
		styledout(const std::string& args);

		styledout& add(const std::string& arg);

		styledout& fg(ColorCode color);
		styledout& fg(color8b color);
		styledout& fg(color8b r, color8b g, color8b b);

		styledout& bg(ColorCode color);
		styledout& bg(color8b color);
		styledout& bg(color8b r, color8b g, color8b b);

		template<typename Type>
		std::ostream& operator << (const Type& obj) {
			return std::cout << *this << obj;
		}

		friend std::ostream& operator << (std::ostream& stream, const styledout& cs);
	private:
		std::string style;
	};
}


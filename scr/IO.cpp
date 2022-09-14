#include "IO.h"

#include <stdexcept>

#include "Board.h"

namespace std
{
	std::string to_string(GGChess::Square square) {
		if (square == GGChess::Square::InvalidSquare)
			return "X";

		std::string str = "a0";
		str[0] = (char)((square) % 8) + 'a';
		str[1] = (char)((square) / 8) + '1';
		return str;
	}

	std::string to_string(GGChess::Piece piece)
	{
		return std::string() + GGChess::piece_to_char(piece);
	}

	std::string to_string(const GGChess::Move& move)
	{
		char promote = 0;
		if (move.flags & GGChess::Move::PromoteQ) promote = 'q';
		else if (move.flags & GGChess::Move::PromoteR) promote = 'r';
		else if (move.flags & GGChess::Move::PromoteN) promote = 'n';
		else if (move.flags & GGChess::Move::PromoteB) promote = 'b';

		std::string str = to_string(move.origin) + std::to_string(move.target);
		if (promote)
			str += promote;

		return str;
	}

	std::string to_string(GGChess::Side side) {
		return side == GGChess::Side::White ? "White" : "Black";
	}
}

namespace GGChess
{
	char piece_to_char(Piece piece)
	{
		char c;

		switch (pieceof(piece)) {
		case GGChess::PieceType::King: c = 'k'; break;
		case GGChess::PieceType::Queen: c = 'q'; break;
		case GGChess::PieceType::Bishop: c = 'b'; break;
		case GGChess::PieceType::Knight: c = 'n'; break;
		case GGChess::PieceType::Rook: c = 'r'; break;
		case GGChess::PieceType::Pawn: c = 'p'; break;
		case GGChess::PieceType::None: c = ' '; break;
		default: throw std::invalid_argument("piece was not a valid type"); break;
		}
		if (sideof(piece) == GGChess::Side::White)
			c = std::toupper(c);

		return c;
	}

	Piece char_to_piece(char c)
	{
		PieceType p;

		switch (std::tolower(c)) {
		case 'k': p = PieceType::King; break;
		case 'q': p = PieceType::Queen; break;
		case 'b': p = PieceType::Bishop; break;
		case 'n': p = PieceType::Knight; break;
		case 'r': p = PieceType::Rook; break;
		case 'p': p = PieceType::Pawn; break;
		default: throw std::invalid_argument("Invalid piece notation"); break;
		}
		Side side = std::isupper(c) ? Side::White : Side::Black;
		return p | side;
	}

	std::ostream& operator << (std::ostream& stream, const Board& board)
	{
		styledout bbs, bws,  wws, wbs;
		bbs.bg(styledout::GRAY).fg(styledout::BLACK);
		bws.bg(styledout::GRAY).fg(styledout::WHITE);
		wws.bg(styledout::LIGHT_GRAY).fg(styledout::WHITE);
		wbs.bg(styledout::LIGHT_GRAY).fg(styledout::BLACK);

		const char* files = "    a  b  c  d  e  f  g  h";

		std::cout << files << std::endl;

		for (int rank = 7; rank >= 0; rank--) 
		{
			std::cout << ' ' << rank + 1 << ' ';

			for (int file = 0; file < 8; file++)
			{
				Square sq = Square(rank * 8 + file);
				styledout style;

				if ((rank + file) % 2 == 0)
					style = sideof(board[sq]) == Side::White ? bws : bbs;
				else
					style = sideof(board[sq]) == Side::White ? wws : wbs;

				style << ' ' << piece_to_char(board[rank * 8 + file]) << ' ';
			}
			std::cout << styledout::nostyle << ' ' << rank + 1 << std::endl;
		}
		std::cout << files << std::endl;

		std::cout << "e.p. target : " << std::to_string(board.EPTarget()) << "; castle : " << (unsigned int)board.Castling() << std::endl;

		return stream;
	}

	std::ostream& operator << (std::ostream& stream, const Move& move)
	{
		stream << std::to_string(move.origin) << std::to_string(move.target);
		switch (move.flags) {
		case Move::Flags::PromoteQ: stream << 'q'; break;
		case Move::Flags::PromoteR: stream << 'r'; break;
		case Move::Flags::PromoteN: stream << 'n'; break;
		case Move::Flags::PromoteB: stream << 'b'; break;
		}
		return stream;
	}

	std::ostream& operator<<(std::ostream& stream, const BitBoard& board)
	{
		for (int i = 7; i >= 0; i--) {
			for (int j = 0; j < 8; j++) {
				stream << board.Get((Square)(i * 8 + j));
			}
			stream << std::endl;
		}
		return stream;
	}

	std::istream& operator >> (std::istream& stream, Square& square)
	{
		char file, rank;
		stream >> file;
		
		if (file == '-')
			square = Square::InvalidSquare;
		else {
			stream >> rank;
			square = (Square)((rank - '1') * 8 + (file - 'a'));
		}
		return stream;
	}

	const styledout styledout::basic = styledout("0");

	std::ostream& styledout::writeWith(const styledout& style) {
		return std::cout << style;
	}

	styledout::styledout() : style() {}

	styledout::styledout(const std::string& args) : style(args) {}

	styledout& styledout::add(const std::string& arg) {
		if (this->style.length() > 0) {
			this->style += ";";
		}
		this->style += arg;
		return *this;
	}

	styledout& styledout::fg(ColorCode color) {
		return this->add(std::to_string(static_cast<int>(color)));
	}

	styledout& styledout::fg(color8b color) {
		return this->add("38").add("5").add(std::to_string(static_cast<int>(color)));
	}

	styledout& styledout::fg(color8b r, color8b g, color8b b) {
		return this->add("38").add("2")
			.add(std::to_string(static_cast<int>(r)))
			.add(std::to_string(static_cast<int>(g)))
			.add(std::to_string(static_cast<int>(b)));
	}

	styledout& styledout::bg(ColorCode color) {
		int temp = static_cast<int>(color) + 10;
		return this->add(std::to_string(static_cast<int>(temp)));
	}

	styledout& styledout::bg(color8b color) {
		return this->add("48").add("5").add(std::to_string(static_cast<int>(color)));
	}

	styledout& styledout::bg(color8b r, color8b g, color8b b) {
		return this->add("48").add("2")
			.add(std::to_string(static_cast<int>(r)))
			.add(std::to_string(static_cast<int>(g)))
			.add(std::to_string(static_cast<int>(b)));
	}

	std::ostream& operator<<(std::ostream& stream, const styledout& cs) {
		stream << "\033[" + cs.style + "m";
		return stream;
	}
}
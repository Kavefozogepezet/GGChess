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
		const char
			*sep1 = " +---+---+---+---+---+---+---+---+ ",
			*sep2 = " | ";

		for (int rank = 7; rank >= 0; rank--) {
			std::cout << std::endl << sep1 << std::endl << sep2;
			for (int file = 0; file < 8; file++) {
				std::cout << piece_to_char(board.GetBoard()[rank * 8 + file]) << sep2;
			}
			std::cout << rank + 1;
		}
		std::cout << std::endl << sep1 << std::endl << "   a   b   c   d   e   f   g   h" << std::endl;

		std::cout << "e.p. target : " << std::to_string(board.GetEnPassantTarget()) << "; castle : " << (unsigned int)board.GetCastleState() << std::endl;

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
}
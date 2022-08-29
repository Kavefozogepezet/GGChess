#include "Fen.h"

#include <sstream>
#include <iostream>

#include "Board.h"
#include "IO.h"

namespace GGChess
{
	std::string Fen::Get(const Board& board)
	{
		return "";
	}

	void Fen::Set(Board& board, std::istream& stream)
	{
		for (size_t i = 0; i < 64; i++)
			board.board[i] = Piece::None;
		
		std::string sec1, sec2, sec3;
		stream >> sec1 >> sec2 >> sec3 >> board.ep_start;
		board.ep_target = board.ep_start;

		int file = 0, rank = 7;

		for (char c : sec1) {
			if (c == '/') {
				rank--;
				file = 0;
			}
			else {
				if (std::isdigit(c))
					file += (int)(c - '0');
				else {
					Square square = (Square)(rank * 8 + file);
					Piece piece = char_to_piece(c);
					board.PlacePiece(square, piece);
					file++;
				}
			}
		}
		board.turn = sec2 == "w" ? Side::White : Side::Black;

		CastleFlag castle = (CastleFlag)0;
		size_t end = std::string::npos;
		if (sec3.find('Q') != end) castle = (CastleFlag)(castle | CastleFlag::WhiteQueenside);
		if (sec3.find('K') != end) castle = (CastleFlag)(castle | CastleFlag::WhiteKingside);
		if (sec3.find('q') != end) castle = (CastleFlag)(castle | CastleFlag::BlackQueenside);
		if (sec3.find('k') != end) castle = (CastleFlag)(castle | CastleFlag::BlackKingside);
		board.castling = castle;
	}

	void Fen::Set(Board& board, const std::string& fen)
	{
		std::stringstream stream(fen);
		Set(board, stream);
	}
}

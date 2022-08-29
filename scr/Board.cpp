#include "Board.h"

#include <stdexcept>

#include <iostream>
#include "IO.h"

#include "MovePatterns.h"

namespace GGChess
{
	Board::Board() :
		board{ Piece::None }, myHash(),
		whiteKing(Square::InvalidSquare),
		blackKing(Square::InvalidSquare),
		turn(Side::White),
		ep_start(Square::InvalidSquare),
		ep_target(Square::InvalidSquare),
		castling((CastleFlag)15),
		halfmove_count(0), moveList(),
		poppedLast(Square::InvalidSquare, Square::InvalidSquare)
	{
		moveList.reserve(16); // TODO max depth option

		Piece pieces[3] = { Piece::Rook, Piece::Knight, Piece::Bishop};
		
		for (char file = 0; file < 3; file++) {
			PlacePiece(Square::a1 + file, pieces[file] | Side::White);
			PlacePiece(Square::h1 - file, pieces[file] | Side::White);

			PlacePiece(Square::a8 + file, pieces[file] | Side::Black);
			PlacePiece(Square::h8 - file, pieces[file] | Side::Black);
		}
		
		for (char file = 0; file < 8; file++) {
			PlacePiece(Square::a2 + file, Piece::Pawn | Side::White);
			PlacePiece(Square::a7 + file, Piece::Pawn | Side::Black);
		}
		
		PlacePiece(Square::e1, Piece::King | Side::White);
		PlacePiece(Square::e8, Piece::King | Side::Black);
		
		PlacePiece(Square::d1, Piece::Queen | Side::White);
		PlacePiece(Square::d8, Piece::Queen | Side::Black);
	}

	void Board::PlayUnrecorded(const Move& move)
	{
		const Move::Flags promote = (Move::Flags)(Move::PromoteQ | Move::PromoteR | Move::PromoteN | Move::PromoteB);

		myHash.castle(castling); // in case it changes we remove it from the hash code

		MovePiece(move.origin, move.target);

		if (move.flags & Move::EnPassant)
			RemovePiece(ep_target + (turn == Side::White ? SDir::S : SDir::N));
		else if (move.flags & promote) {
			Piece promoteTo = Piece::Queen;
			Side promoteSide = sideof(board[move.target]);

			switch (move.flags) {
			case Move::PromoteR: promoteTo = Piece::Rook; break;
			case Move::PromoteN: promoteTo = Piece::Knight; break;
			case Move::PromoteB: promoteTo = Piece::Bishop; break;
			}
			RemovePiece(move.target);
			PlacePiece(move.target, promoteTo | promoteSide);
			//board[move.target] = promoteTo | sideof(board[move.target]);
		}
		else if (move.flags & Move::Castle) {
			Square rookSquare = Square::a1;
			if (move.target > move.origin)
				rookSquare = (Square)(rankof(move.origin) * 8 + 7);
			else
				rookSquare = (Square)(rankof(move.origin) * 8);

			MovePiece(rookSquare, (Square)((move.target + move.origin) / 2));

			if (sideof(board[move.target]) == Side::White)
				castling = (CastleFlag)(castling & ~(CastleFlag::WhiteKingside | CastleFlag::WhiteQueenside));
			else
				castling = (CastleFlag)(castling & ~(CastleFlag::BlackKingside | CastleFlag::BlackQueenside)); 
		}

		if (ep_target != Square::InvalidSquare) { // clearing enpassant target
			myHash.enPassant(ep_target);
			ep_target = Square::InvalidSquare;
		}
		if (move.flags & Move::DoublePush) { // writing en passant target
			ep_target = move.origin + (turn == Side::White ? SDir::N : SDir::S);
			myHash.enPassant(ep_target);
		}

		Square rookSquare = Square::InvalidSquare;
		// castling state
		if (pieceof(board[move.target]) == Piece::Rook) {
			rookSquare = move.origin;
		}
		else if (pieceof(move.captured) == Piece::Rook) {
			rookSquare = move.target;
		}
		else if (pieceof(board[move.target]) == Piece::King) {
			if(move.origin == Square::e1)
				castling = (CastleFlag)(castling & ~(CastleFlag::WhiteKingside | CastleFlag::WhiteQueenside));
			else if(move.origin == Square::e8)
				castling = (CastleFlag)(castling & ~(CastleFlag::BlackKingside | CastleFlag::BlackQueenside));
		}

		if (rookSquare != Square::InvalidSquare) {
			if (rookSquare == Square::a1)
				castling = (CastleFlag)(castling & ~(CastleFlag::WhiteQueenside));
			else if (rookSquare == Square::h1)
				castling = (CastleFlag)(castling & ~(CastleFlag::WhiteKingside));
			else if (rookSquare == Square::a8)
				castling = (CastleFlag)(castling & ~(CastleFlag::BlackQueenside));
			else if (rookSquare == Square::h8)
				castling = (CastleFlag)(castling & ~(CastleFlag::BlackKingside));
		}

		myHash.castle(castling); // add the new castling state

		turn = OtherSide(turn);
		myHash.flipSide();
	}

	void Board::PlayMove(const Move& move) {
		CastleFlag castleState = castling;
		PlayUnrecorded(move);
		moveList.push_back({ move, castleState });
	}

	void Board::UnplayMove()
	{
		const Move::Flags promote = (Move::Flags)(Move::PromoteQ | Move::PromoteR | Move::PromoteN | Move::PromoteB);
		const Move& move = moveList.back().move;

		MovePiece(move.target, move.origin);
		if (pieceof(move.captured) != Piece::None)
			PlacePiece(move.target, move.captured);

		if (move.flags & promote) {
			Side promoteSide = sideof(board[move.origin]);
			RemovePiece(move.origin);
			PlacePiece(move.origin, Piece::Pawn | promoteSide);
			//board[move.origin] = Piece::Pawn | sideof(board[move.origin]);
		}
		else if (move.flags & Move::EnPassant) {
			Square pawnSquare =
				fileof(move.target) - fileof(move.origin) > 0 ?
				move.origin + SDir::E :
				move.origin + SDir::W;
			PlacePiece(pawnSquare, Piece::Pawn | OtherSide(sideof(board[move.origin])));
		}
		else if (move.flags & Move::Castle) {
			Square
				rookSquare = (Square)((move.target + move.origin) / 2),
				rookTarget =
					fileof(rookSquare) < 4 ?
					(Square)(rankof(rookSquare) * 8) :
					(Square)(rankof(rookSquare) * 8 + 7);
			MovePiece(rookSquare, rookTarget);
		}

		myHash.castle(castling);
		castling = moveList.back().castle;
		myHash.castle(castling);

		poppedLast = moveList.back().move;
		moveList.pop_back();
		turn = OtherSide(turn);
		myHash.flipSide();

		//en passant
		if (ep_target != Square::InvalidSquare) { // clearing enpassant target
			myHash.enPassant(ep_target);
			ep_target = Square::InvalidSquare;
		}

		if (moveList.size() == 0) {
			ep_target = ep_start;
			myHash.enPassant(ep_target);
		}
		else if (moveList.back().move.flags & Move::DoublePush) {
			ep_target = moveList.back().move.origin + (turn == Side::White ? SDir::S : SDir::N); //moveList.back().first.target;
			myHash.enPassant(ep_target);
		}
	}

	const Piece& Board::operator [] (Square square) const {
		if (!validSquare(square))
			throw std::out_of_range("square was invalid");
		return board[square];
	}

	const Piece& Board::operator [] (size_t idx) const {
		return operator [] ((Square)idx);
	}

	const Piece& Board::at(Square square) const {
		if (!validSquare(square))
			throw std::out_of_range("square was invalid");
		return board[square];
	}

	const Piece& Board::at(size_t idx) const {
		return at((Square)idx);
	}

	Square Board::GetKing(Side side) const {
		return side == Side::White ? whiteKing : blackKing;
	}

	Side Board::GetTurn() const {
		return turn;
	}

	const std::array<Piece, 64>& Board::GetBoard() const {
		return board;
	}

	Square Board::GetEnPassantTarget() const {
		return ep_target;
	}

	bool Board::CanCastle(CastleFlag flag) const {
		return castling & flag;
	}

	CastleFlag Board::GetCastleState() const {
		return castling;
	}

	int Board::GetHalfMoves() const {
		return halfmove_count;
	}

	void Board::SetThisAsStart() {
		ep_start = ep_target;
		moveList.clear();
	}

	void Board::PlacePiece(Square square, Piece piece)
	{
		board[square] = piece;
		myHash.piece(piece, square);

		bool king = pieceof(piece) == Piece::King;

		if (sideof(piece) == Side::White) {
			//white.insert(square);
			if (king)
				whiteKing = square;
		}
		else {
			//black.insert(square);
			if (king)
				blackKing = square;
		}
	}

	void Board::RemovePiece(Square square)
	{
		Piece piece = board[square];
		myHash.piece(piece, square);

		bool isWhite = sideof(piece) == Side::White;

		if (pieceof(board[square]) == Piece::King)
			if (isWhite)
				whiteKing = Square::InvalidSquare;
			else
				blackKing = Square::InvalidSquare;

		board[square] = Piece::None;
	}

	void Board::MovePiece(Square origin, Square target)
	{
		Piece p = board[origin];
		Piece t = board[target];

		if (pieceof(p) == Piece::None)
			throw std::invalid_argument("No piece was standing at the origin square");

		if (pieceof(t) != Piece::None && sideof(t) != sideof(p))
			RemovePiece(target);
		else if (pieceof(t) != Piece::None && sideof(t) == sideof(p))
			throw std::invalid_argument("Attempted to take same color");

		RemovePiece(origin);
		PlacePiece(target, p);
	}

	PosInfo Board::GetPosInfo() const
	{
		PosInfo info;
		Side attacker = OtherSide(turn);

		Square square = GetKing(turn);
		int checkCount = 0;

		KnightPattern(square, [&](Square target) {
			if (pieceof(board[target]) == Piece::Knight && sideof(board[target]) == attacker) {
				info.checkBoard.Set(target, true);
				checkCount++;
			}
			return true;
			});

		PawnPattern(square, turn, [&](Square target, bool attack) {
			if (pieceof(board[target]) == Piece::Pawn && sideof(board[target]) == attacker) {
				info.checkBoard.Set(target, true);
				checkCount++;
			}
			return true;
			}, true);

		BitBoard current;
		int myPieceCount = 0;
		int lastIdx = 0;

		SlidingPiecePattern(square, Piece::Queen, [&](Square target, int rayIdx) {
			if (rayIdx != lastIdx) {
				current.bits = 0;
				myPieceCount = 0;
				lastIdx = rayIdx;
			}
			Piece targetPiece = board[target];
			current.Set(target, true);

			if (pieceof(targetPiece) != Piece::None) {
				if (sideof(targetPiece) == turn)
					myPieceCount++;
				else if (sideof(targetPiece) == attacker) {
					Piece p = pieceof(targetPiece);
					bool canAttack =
						p == Piece::Queen ||
						(p == Piece::Bishop && rayIdx % 2 == 1) ||
						(p == Piece::Rook && rayIdx % 2 == 0);

					if (canAttack) {
						if (myPieceCount == 0) {
							info.checkBoard.Set(current.bits, true);
							checkCount++;
						}
						else if (myPieceCount == 1)
							info.pinBoards[rayIdx].Set(current.bits, true);
					}
					return false;
				}
			}
			if (myPieceCount > 1)
				return false;
			return true;
			});

		if (checkCount > 0)
			info.check = true;
		if (checkCount > 1)
			info.doubleCheck = true;

		for (BitBoard& pin : info.pinBoards)
			info.unifiedPinBoard.Set(pin.bits, true);

		return info;
	}

	ZobristKey Board::GetPosKey() const {
		return myHash.key();
	}
}

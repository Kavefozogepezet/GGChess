#include "Board.h"

#include <stdexcept>

#include <iostream>
#include "IO.h"

#include "MovePatterns.h"

namespace GGChess
{
	Board::Board() :
		board{ Piece::Empty }, hash(),
		whiteKing(Square::InvalidSquare),
		blackKing(Square::InvalidSquare),
		turn(Side::White),
		ep_start(Square::InvalidSquare),
		ep_target(Square::InvalidSquare),
		castling((CastleFlag)15),
		ply(0), moveRecord()
	{
		PieceType pieces[3] = { PieceType::Rook, PieceType::Knight, PieceType::Bishop};
		
		for (int8_t file = 0; file < 3; file++) {
			PlacePiece(Square::a1 + file, pieces[file] | Side::White);
			PlacePiece(Square::h1 - file, pieces[file] | Side::White);

			PlacePiece(Square::a8 + file, pieces[file] | Side::Black);
			PlacePiece(Square::h8 - file, pieces[file] | Side::Black);
		}
		
		for (int8_t file = 0; file < 8; file++) {
			PlacePiece(Square::a2 + file, Piece::WPawn);
			PlacePiece(Square::a7 + file, Piece::BPawn);
		}
		
		PlacePiece(Square::e1, Piece::WKing);
		PlacePiece(Square::e8, Piece::BKing);
		
		PlacePiece(Square::d1, Piece::WQueen);
		PlacePiece(Square::d8, Piece::BQueen);
	}

	void Board::PlayUnrecorded(const Move& move)
	{
		hash.castle(castling); // in case it changes we remove it from the hash code

		Piece mPiece = board[move.origin]; // the moveing piece
		PieceType mPieceT = pieceof(mPiece);

		MovePiece(move.origin, move.target);

		if (move.flags & Move::EnPassant)
			RemovePiece(ep_target + (turn == Side::White ? SDir::S : SDir::N));
		else if (move.flags & Move::Flags::Promotion) {
			PieceType promoteTo = PieceType::Queen;

			switch (move.flags) {
			case Move::PromoteR: promoteTo = PieceType::Rook; break;
			case Move::PromoteN: promoteTo = PieceType::Knight; break;
			case Move::PromoteB: promoteTo = PieceType::Bishop; break;
			}
			RemovePiece(move.target);
			PlacePiece(move.target, promoteTo | turn);
		}
		else if (move.flags & Move::Castle) {
			Square rookSquare = Square::a1;
			if (move.target > move.origin)
				rookSquare = Square(rankof(move.origin) * 8 + 7);
			else
				rookSquare = Square(rankof(move.origin) * 8);

			MovePiece(rookSquare, Square((move.target + move.origin) / 2));

			if (turn == Side::White)
				castling &= CastleFlag::BlackCastle; // clear white castle ability
			else
				castling &= CastleFlag::WhiteCastle; // clear black castle ability
		}

		if (ep_target != Square::InvalidSquare) { // clearing enpassant target
			hash.enPassant(ep_target);
			ep_target = Square::InvalidSquare;
		}
		if (move.flags & Move::DoublePush) { // writing en passant target
			ep_target = move.origin + (turn == Side::White ? SDir::N : SDir::S);
			hash.enPassant(ep_target);
		}

		// castling state
		if (mPiece == Piece::WRook) { // white rook moved
			if (move.origin == Square::a1)
				castling &= ~CastleFlag::WhiteQueenside;
			else if (move.origin == Square::h1)
				castling &= ~CastleFlag::WhiteKingside;
		}
		else if(mPiece == Piece::BRook) { // black rook moved
			if (move.origin == Square::a8)
				castling &= ~CastleFlag::BlackQueenside;
			else if (move.origin == Square::h8)
				castling &= ~CastleFlag::BlackKingside;
		}
		else if (mPiece == Piece::WKing) {
			castling &= CastleFlag::BlackCastle; // clear white castle ability
		}
		else if (mPiece == Piece::BKing) {
			castling &= CastleFlag::WhiteCastle; // clear black castle ability
		}
		
		if (move.captured == Piece::WRook) { // white rook captured
			if (move.target == Square::a1)
				castling &= ~CastleFlag::WhiteQueenside;
			else if (move.target == Square::h1)
				castling &= ~CastleFlag::WhiteKingside;
		}
		else if (move.captured == Piece::BRook) { // black rook captured
			if (move.target == Square::a8)
				castling &= ~CastleFlag::BlackQueenside;
			else if (move.target == Square::h8)
				castling &= ~CastleFlag::BlackKingside;
		}
		
		hash.castle(castling); // add the new castling state

		turn = otherside(turn);
		hash.flipSide();
	}

	void Board::PlayMove(const Move& move) {
		CastleFlag castleState = castling;
		PlayUnrecorded(move);
		moveRecord.push_back({ move, castleState });
	}

	void Board::UnplayMove()
	{
		MoveData moveData = moveRecord.pop_back();
		const Move& move = moveData.move;
		turn = otherside(turn);
		hash.flipSide();

		MovePiece(move.target, move.origin);

		if (move.captured != Piece::Empty)
			PlacePiece(move.target, move.captured);

		if (move.flags & Move::Flags::Promotion) {
			RemovePiece(move.origin);
			PlacePiece(move.origin, PieceType::Pawn | turn);
		}
		else if (move.flags & Move::EnPassant) {
			Square pawnSquare =
				fileof(move.target) - fileof(move.origin) > 0 ?
				move.origin + SDir::E :
				move.origin + SDir::W;
			PlacePiece(pawnSquare, PieceType::Pawn | otherside(turn));
		}
		else if (move.flags & Move::Castle) {
			Square
				rookSquare = (Square)((move.target + move.origin) / 2),
				rookTarget =
					fileof(rookSquare) < 4 ?
					Square(rankof(rookSquare) * 8) :
					Square(rankof(rookSquare) * 8 + 7);
			MovePiece(rookSquare, rookTarget);
		}

		hash.castle(castling);
		castling = moveData.castle;
		hash.castle(castling);

		//en passant
		if (ep_target != Square::InvalidSquare) { // clearing enpassant target
			hash.enPassant(ep_target);
			ep_target = Square::InvalidSquare;
		}

		if (moveRecord.size() == 0) {
			ep_target = ep_start;
			hash.enPassant(ep_target);
		}
		else if (moveRecord.back().move.flags & Move::DoublePush) {
			ep_target = moveRecord.back().move.origin + (turn == Side::White ? SDir::S : SDir::N);
			hash.enPassant(ep_target);
		}
	}

	const Piece& Board::operator [] (Square square) const { // TODO call array op.
		if (!validsquare(square))
			throw std::out_of_range("square was invalid");
		return board[square];
	}

	const Piece& Board::operator [] (size_t idx) const {
		return operator [] ((Square)idx);
	}

	const Piece& Board::at(Square square) const {
		return board.at(square);
	}

	const Piece& Board::at(size_t idx) const {
		return board.at(idx);
	}

	Square Board::King(Side side) const {
		return side == Side::White ? whiteKing : blackKing;
	}

	Side Board::Turn() const {
		return turn;
	}

	const std::array<Piece, 64>& Board::array() const {
		return board;
	}

	Square Board::EPTarget() const {
		return ep_target;
	}

	bool Board::CanCastle(CastleFlag flag) const {
		return castling & flag;
	}

	CastleFlag Board::Castling() const {
		return castling;
	}

	int Board::Ply() const {
		return ply;
	}

	void Board::SetThisAsStart() {
		ep_start = ep_target;
		moveRecord.clear();
	}

	void Board::PlacePiece(Square square, Piece piece)
	{
		board[square] = piece;
		hash.piece(piece, square);

		if (pieceof(piece) == PieceType::Pawn)
			phash.piece(piece, square);

		if (piece == Piece::WKing)
			whiteKing = square;
		else if (piece == Piece::BKing)
			blackKing = square;
	}

	void Board::RemovePiece(Square square)
	{
		Piece piece = board[square];
		hash.piece(piece, square);

		if (pieceof(piece) == PieceType::Pawn)
			phash.piece(piece, square);

		if (piece == Piece::WKing)
			whiteKing = Square::InvalidSquare;
		else if (piece == Piece::BKing)
			blackKing = Square::InvalidSquare;

		board[square] = Piece::Empty;
	}

	void Board::MovePiece(Square origin, Square target)
	{
		Piece p = board[origin];
		Piece t = board[target];

		if (p == Piece::Empty)
			throw std::invalid_argument("No piece was standing at the origin square");

		if (t != Piece::Empty) {
			if (sideof(t) != sideof(p))
				RemovePiece(target);
			else
				throw std::invalid_argument("Attempted to take same color");
		}

		RemovePiece(origin);
		PlacePiece(target, p);
	}

	PosInfo Board::Info() const
	{
		PosInfo info;
		Side attacker = otherside(turn);

		Square square = King(turn);
		int checkCount = 0;

		KnightPattern(square, [&](Square target) {
			if (pieceof(board[target]) == PieceType::Knight && sideof(board[target]) == attacker) {
				info.checkBoard.Set(target, true);
				checkCount++;
			}
			return true;
			});

		PawnPattern(square, turn, [&](Square target, bool attack) {
			if (pieceof(board[target]) == PieceType::Pawn && sideof(board[target]) == attacker) {
				info.checkBoard.Set(target, true);
				checkCount++;
			}
			return true;
			}, true);

		BitBoard current;
		int myPieceCount = 0;
		int lastIdx = 0;

		SlidingPiecePattern(square, PieceType::Queen, [&](Square target, int rayIdx) {
			if (rayIdx != lastIdx) {
				current.bits = 0;
				myPieceCount = 0;
				lastIdx = rayIdx;
			}
			Piece targetPiece = board[target];
			current.Set(target, true);

			if (targetPiece != Piece::Empty) {
				if (sideof(targetPiece) == turn)
					myPieceCount++;
				else {
					PieceType p = pieceof(targetPiece);
					bool canAttack =
						p == PieceType::Queen ||
						(p == PieceType::Bishop && rayIdx % 2 == 1) ||
						(p == PieceType::Rook && rayIdx % 2 == 0);

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

		// attack board
		BitBoard pbf, pbo;

		for (int8_t i = 0; i < BOARD_SQUARE_COUNT; i++) {
			Piece p = board[i];

			if (p == Piece::Empty || sideof(p) != attacker) {
				if (pieceof(p) == PieceType::Pawn)
					pbf.Set(Square(i), true);
				continue;
			}

			switch (pieceof(p)) {
			case PieceType::Pawn:
				pbo.Set(Square(i), true);
				break;
			case PieceType::Knight:
				KnightPattern(Square(i), [&](Square target) {
					info.attackBoard.Set(target, true);
					return true;
					});
				break;
			case PieceType::King:
				SlidingPiecePattern(Square(i), PieceType::Queen, [&](Square target, int rayIdx) {
					info.attackBoard.Set(target, true);
					return false;
					});
				break;
			case PieceType::Bishop:
			case PieceType::Rook:
			case PieceType::Queen:
				SlidingPiecePattern(Square(i), pieceof(p), [&](Square target, int rayIdx) {
					info.attackBoard.Set(target, true);
					return (board[target] == Piece::Empty) || target == King(turn);
					});
				break;
			}
		}

		const uint64_t
			fileA = 0x0101010101010101ULL,
			fileH = fileA << 7;

		size_t idx = turn == Side::White ? 0 : 1;
		info.pAttackBoard[idx] = attacker == Side::Black ?
			(pbf.bits & ~fileA) << 7 | (pbf.bits & ~fileH) << 9 :
			(pbf.bits & ~fileA) >> 9 | (pbf.bits & ~fileH) >> 7;

		idx = turn == Side::White ? 1 : 0;
		info.pAttackBoard[idx] = attacker == Side::White ?
			(pbo.bits & ~fileA) << 7 | (pbo.bits & ~fileH) << 9 :
			(pbo.bits & ~fileA) >> 9 | (pbo.bits & ~fileH) >> 7;


		info.attackBoard.Set(info.pAttackBoard[idx].bits, true);

		return info;
	}

	ZobristKey Board::Key() const {
		return hash.key();
	}

	ZobristKey Board::PKey() const {
		return phash.key();
	}
}

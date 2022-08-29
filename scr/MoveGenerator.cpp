#include "MoveGenerator.h"

#include <cmath>

#include <iostream>
#include "IO.h"

#include "Board.h"
#include "MovePatterns.h"

namespace GGChess
{
	static SDir const knightMoves[8] = { (SDir)17, (SDir)10, (SDir)-6, (SDir)-15, (SDir)-17, (SDir)-10, (SDir)6, (SDir)15 };
	static SDir const directions[8] = { SDir::N, SDir::NE, SDir::E, SDir::SE, SDir::S, SDir::SW, SDir::W, SDir::NW };
	
	MoveList::MoveList() :
		moveArray(), last(moveArray), count(0)
	{}

	size_t MoveList::size() const { return count; }

	void MoveList::push_back(const Move& move) {
		count++;
		*last++ = move;
	}

	Move& MoveList::operator[](size_t idx) { return moveArray[idx]; }

	Move* MoveList::begin() { return moveArray; }

	Move* MoveList::end() { return last; }

	bool AddIfLegal(Board& board, const PosInfo& info, const Move& move, MoveList& moves)
	{
		bool kingIsMoving = move.origin == board.GetKing(board.GetTurn());
		if (info.doubleCheck && !kingIsMoving)
			return false;

		if (move.flags == Move::Flags::EnPassant || move.origin == board.GetKing(board.GetTurn())) {
			board.PlayMove(move);

			Square kingSquare = board.GetKing(OtherSide(board.GetTurn()));
			bool legalFlag = false;

			if (!IsSquareAttacked(board, info, kingSquare, board.GetTurn()))
				legalFlag = true;

			board.UnplayMove();

			if (legalFlag)
				moves.push_back(move);

			return legalFlag;
		}
		if (info.check) {
			if (!info.checkBoard.Get(move.target))
				return false;
		}
		if (info.unifiedPinBoard.Get(move.origin)) {
			for (BitBoard pin : info.pinBoards) {
				if (!pin.Get(move.origin))
					continue;

				if (!pin.Get(move.target))
					return false;
				else
					break;
			}
		}
		moves.push_back(move);
		return true;
	}

	bool IsSquareAttacked(Board& board, const PosInfo& info, Square square, Side attacker)
	{
		if (square == Square::InvalidSquare)
			throw std::invalid_argument("square was invalid");

		Side side = OtherSide(attacker);
		bool isAttacked = false;

		KnightPattern(square, [&](Square target) {
			if (pieceof(board[target]) == Piece::Knight && sideof(board[target]) == attacker) {
				isAttacked = true;
				return false;
			}
			return true;
			});

		PawnPattern(square, side, [&](Square target, bool attack) {
			if (pieceof(board[target]) == Piece::Pawn && sideof(board[target]) == attacker)
				isAttacked = true;
			return true;
			}, true);

		if (isAttacked)
			return true;

		SlidingPiecePattern(square, Piece::Queen, [&](Square target, int rayIdx) {
			Piece targetPiece = board[target];

			if (pieceof(targetPiece) != Piece::None) {
				if (sideof(targetPiece) == attacker) {
					Piece p = pieceof(targetPiece);
					if( p == Piece::Queen ||
						(p == Piece::Bishop && rayIdx % 2 == 1) ||
						(p == Piece::Rook && rayIdx % 2 == 0))
						isAttacked = true;
				}
				return false;
			}
			return true;
			});

		SlidingPiecePattern(square, Piece::Queen, [&](Square target, int rayIdx) {
			Piece targetPiece = board[target];
			if (pieceof(targetPiece) == Piece::King && sideof(targetPiece) == attacker)
				isAttacked = true;
			return false;
			});

		if (isAttacked)
			return true;
		return false;
	}

	void AddWithPromotion(Board& board, const PosInfo& info, Square origin, Square target, Piece capture, MoveList& moves)
	{
		int rank = rankof(target);
		if (rank == 7 || rank == 0) {
			if (AddIfLegal(board, info, Move(origin, target, capture, Move::PromoteQ), moves)) {
				moves.push_back(Move(origin, target, capture, Move::PromoteR));
				moves.push_back(Move(origin, target, capture, Move::PromoteN));
				moves.push_back(Move(origin, target, capture, Move::PromoteB));
			}
		}
		else {
			AddIfLegal(board, info, Move(origin, target, capture), moves);
		}
	}

	void GeneratePawnMoves(Board& board, const PosInfo& info, Square square, Side side, MoveList& moves)
	{
		PawnPattern(square, side, [&](Square target, bool attack) {
			Piece targetPiece = board[target];
			if (attack) {
				if (pieceof(targetPiece) != Piece::None && sideof(targetPiece) != side)
					AddWithPromotion(board, info, square, target, targetPiece, moves);
				else if(target == board.GetEnPassantTarget())
					AddIfLegal(board, info, Move(square, target, Move::EnPassant), moves);
			}
			else {
				if (pieceof(targetPiece) != Piece::None)
					return false;
				if(std::abs(target - square) == 16)
					AddIfLegal(board, info, Move(square, target, Move::DoublePush), moves);
				else
					AddWithPromotion(board, info, square, target, targetPiece, moves);
			}
			return true;
			});
	}

	void GenerateKnightMoves(Board& board, const PosInfo& info, Square square, Side side, MoveList& moves)
	{
		KnightPattern(square, [&](Square target) {
			if (sideof(board[target]) != side)
				AddIfLegal(board, info, Move(square, target, board[target]), moves);
			return true;
			});
	}

	void GenerateSlidingPieceMoves(Board& board, const PosInfo& info, Square square, Piece piece, MoveList& moves)
	{
		Side side = sideof(piece);
		piece = pieceof(piece);

		SlidingPiecePattern(square, piece, [&](Square target, int rayIdx) {
			Piece targetPiece = board[target];

			if (targetPiece == Piece::None)
				AddIfLegal(board, info, Move(square, target), moves);
			else if (sideof(targetPiece) == side)
				return false;
			else if (sideof(targetPiece) != side) {
				AddIfLegal(board, info, Move(square, target, board[target]), moves);
				return false;
			}
			return true;
			});
	}

	void GenerateKingMoves(Board& board, const PosInfo& info, Square square, Side side, MoveList& moves)
	{
		int
			file = fileof(square),
			rank = rankof(square);

		int bounds[4] = { rank == 0 ? 0 : -1, rank == 7 ? 0 : 1, file == 0 ? 0 : -1, file == 7 ? 0 : 1, };

		for (int dr = bounds[0]; dr <= bounds[1]; dr++) {
			for (int df = bounds[2]; df <= bounds[3]; df++) {
				if (dr == 0 && df == 0)
					continue;

				Square target = square + SDir(dr * 8 + df);
				Piece t = board[target];

				if (pieceof(t) != Piece::None && sideof(t) == side)
					continue;

				AddIfLegal(board, info, Move(square, target, t), moves);
			}
		}

		Side attacker = OtherSide(side);
		CastleFlag
			kingside = side == Side::White ? CastleFlag::WhiteKingside : CastleFlag::BlackKingside,
			queenside = side == Side::White ? CastleFlag::WhiteQueenside : CastleFlag::BlackQueenside;

		if (!info.check) {
			if (board.CanCastle(kingside)) {
				bool emptyMid =
					pieceof(board[square + 1]) == Piece::None &&
					pieceof(board[square + 2]) == Piece::None;
				if (emptyMid) {
					bool noAttack =
						!IsSquareAttacked(board, info, square + 1, attacker) &&
						!IsSquareAttacked(board, info, square + 2, attacker);
					if (noAttack)
						moves.push_back(Move(square, square + 2, Move::Castle));
				}
			}
			if (board.CanCastle(queenside)) {
				bool emptyMid =
					pieceof(board[square - 1]) == Piece::None &&
					pieceof(board[square - 2]) == Piece::None &&
					pieceof(board[square - 3]) == Piece::None;
				if (emptyMid) {
					bool noAttack =
						!IsSquareAttacked(board, info, square - 1, attacker) &&
						!IsSquareAttacked(board, info, square - 2, attacker);
					if (noAttack)
						moves.push_back(Move(square, square - 2, Move::Castle));
				}
			}
		}
	}

	void GetAllMoves(Board& board, const PosInfo& info, MoveList& moves)
	{
		for (int i = 0; i < BOARD_SQUARE_COUNT; i++) {
			Piece p = board[(Square)i];
			if (pieceof(p) != Piece::None && sideof(p) == board.GetTurn())
				GetMoves(board, info, (Square)i, moves);
		}
	}

	void GetMoves(Board& board, const PosInfo& info, Square square, MoveList& moves)
	{
		Piece piece = board[square];

		switch (pieceof(piece)) {
		case Piece::Pawn: GeneratePawnMoves(board, info, square, sideof(piece), moves); break;
		case Piece::Knight: GenerateKnightMoves(board, info, square, sideof(piece), moves); break;
		case Piece::King: GenerateKingMoves(board, info, square, sideof(piece), moves); break;
		case Piece::Rook:
		case Piece::Bishop:
		case Piece::Queen:
			GenerateSlidingPieceMoves(board, info, square, piece, moves); break;
		}
	}
}
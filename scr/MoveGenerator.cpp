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

	bool AddIfLegal(Board& board, const PosInfo& info, const Move& move, MoveList& moves)
	{
		Square
			epPawn = board.EPTarget() + (board.Turn() == Side::White ? SDir::S : SDir::W),
			king = board.King(board.Turn());
		bool kingIsMoving = move.origin == king;

		if (info.doubleCheck && !kingIsMoving)
			return false;

		if (move.origin == board.King(board.Turn())) {
			if (info.attackBoard.Get(move.target))
				return false;
			else {
				moves.push_back(move);
				return true;
			}
		}
		
		if (move.flags == Move::Flags::EnPassant) {
			uint8_t
				abs_dr = std::abs(rankof(epPawn) - rankof(king)),
				abs_df = std::abs(fileof(epPawn) - fileof(king));

			bool diag = abs_dr == abs_df;

			if (diag || abs_dr == 0) { // the king can be attacked by sliding piece behind epPawn
				SDir dir = SDir((epPawn - king) / std::max(abs_dr, abs_df));
				Square target = king + dir;
				while (fileof(target) != 0 && fileof(target) != 7 && validsquare(target)) {
					Piece tp = board[target];
					if (tp == Piece::Empty || target == move.origin)
						target = target + dir;
					else if (sideof(tp) == board.Turn())
						break;
					else {
						PieceType tpt = pieceof(tp);
						if (tpt == PieceType::Queen ||
							(tpt == PieceType::Bishop && diag) ||
							(tpt == PieceType::Rook && !diag)) {
							return false;
						}
						else
							break;
					}
				}
			}
		}

		if (info.check) {
			if (!info.checkBoard.Get(move.target) && // the move blocks the check
				!(move.flags == Move::Flags::EnPassant && // the move captures en passant the checking pawn
					info.checkBoard.Get(epPawn)))
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

		Side side = otherside(attacker);
		bool isAttacked = false;

		KnightPattern(square, [&](Square target) {
			if (pieceof(board[target]) == PieceType::Knight && sideof(board[target]) == attacker) {
				isAttacked = true;
				return false;
			}
			return true;
			});

		PawnPattern(square, side, [&](Square target, bool attack) {
			if (pieceof(board[target]) == PieceType::Pawn && sideof(board[target]) == attacker)
				isAttacked = true;
			return true;
			}, true);

		if (isAttacked)
			return true;

		SlidingPiecePattern(square, PieceType::Queen, [&](Square target, int rayIdx) {
			Piece targetPiece = board[target];

			if (targetPiece != Piece::Empty) {
				if (sideof(targetPiece) == attacker) {
					PieceType p = pieceof(targetPiece);
					if( p == PieceType::Queen ||
						(p == PieceType::Bishop && rayIdx % 2 == 1) ||
						(p == PieceType::Rook && rayIdx % 2 == 0))
						isAttacked = true;
				}
				return false;
			}
			return true;
			});

		SlidingPiecePattern(square, PieceType::Queen, [&](Square target, int rayIdx) {
			Piece targetPiece = board[target];
			if (pieceof(targetPiece) == PieceType::King && sideof(targetPiece) == attacker)
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

				if (t != Piece::Empty && sideof(t) == side)
					continue;

				AddIfLegal(board, info, Move(square, target, t), moves);
			}
		}

		Side attacker = otherside(side);
		CastleFlag
			kingside = side == Side::White ? CastleFlag::WhiteKingside : CastleFlag::BlackKingside,
			queenside = side == Side::White ? CastleFlag::WhiteQueenside : CastleFlag::BlackQueenside;

		if (!info.check) {
			if (board.CanCastle(kingside)) {
				bool emptyMid =
					board[square + 1] == Piece::Empty &&
					board[square + 2] == Piece::Empty;
				if (emptyMid) {
					bool noAttack =
						!info.attackBoard.Get(square + 1) &&
						!info.attackBoard.Get(square + 2);
					if (noAttack)
						moves.push_back(Move(square, square + 2, Move::Castle));
				}
			}
			if (board.CanCastle(queenside)) {
				bool emptyMid =
					board[square - 1] == Piece::Empty &&
					board[square - 2] == Piece::Empty &&
					board[square - 3] == Piece::Empty;
				if (emptyMid) {
					bool noAttack =
						!info.attackBoard.Get(square - 1) &&
						!info.attackBoard.Get(square - 2);
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
			if (p != Piece::Empty && sideof(p) == board.Turn())
				GetMoves(board, info, (Square)i, moves);
		}
	}

	void GetAllCaptures(Board& board, const PosInfo& info, MoveList& moves)
	{
		for (int i = 0; i < BOARD_SQUARE_COUNT; i++) {
			Piece p = board[(Square)i];
			PieceType pt = pieceof(p);
			Side ps = sideof(p);

			if (p != Piece::Empty && sideof(p) == board.Turn())
			{
				switch (pt) {
				case PieceType::Pawn:
					PawnPattern(Square(i), ps, [&](Square target, bool attack) {
						Piece tp = board[target];
						if (tp != Piece::Empty && sideof(tp) != ps)
							AddWithPromotion(board, info, Square(i), target, tp, moves);
						return true;
						}, true);
					break;
				case PieceType::Knight:
					KnightPattern(Square(i), [&](Square target) {
						Piece tp = board[target];
						if (tp != Piece::Empty && sideof(tp) != ps)
							AddIfLegal(board, info, Move(Square(i), target, tp), moves);
						return true;
						});
					break;
				case PieceType::King:
					SlidingPiecePattern(Square(i), PieceType::Queen, [&](Square target, int rayIdx) {
						Piece tp = board[target];
						if (tp != Piece::Empty && sideof(tp) != ps)
							AddIfLegal(board, info, Move(Square(i), target, tp), moves);
						return false;
						});
					break;
				case PieceType::Rook:
				case PieceType::Bishop:
				case PieceType::Queen:
					SlidingPiecePattern(Square(i), pt, [&](Square target, int rayIdx) {
						Piece targetPiece = board[target];
						if (targetPiece == Piece::Empty)
							return true;
						if (sideof(targetPiece) == ps)
							return false;
						else if (sideof(targetPiece) != ps) {
							AddIfLegal(board, info, Move(Square(i), target, board[target]), moves);
							return false;
						}
						return true;
						});					
					break;
				}
			}
		}

	}

	void GetMoves(Board& board, const PosInfo& info, Square square, MoveList& moves)
	{
		Piece piece = board[square];
		Side side = sideof(piece);

		switch (pieceof(piece)) {
		case PieceType::Pawn:
			PawnPattern(square, side, [&](Square target, bool attack) {
				Piece targetPiece = board[target];
				if (attack) {
					if (targetPiece != Piece::Empty && sideof(targetPiece) != side)
						AddWithPromotion(board, info, square, target, targetPiece, moves);
					else if (target == board.EPTarget())
						AddIfLegal(board, info, Move(square, target, Move::EnPassant), moves);
				}
				else {
					if (targetPiece != Piece::Empty)
						return false;
					if (std::abs(target - square) == 16)
						AddIfLegal(board, info, Move(square, target, Move::DoublePush), moves);
					else
						AddWithPromotion(board, info, square, target, targetPiece, moves);
				}
				return true;
				});
			break;
		case PieceType::Knight:
			KnightPattern(square, [&](Square target) {
				if (board[target] == Piece::Empty || sideof(board[target]) != side)
					AddIfLegal(board, info, Move(square, target, board[target]), moves);
				return true;
				});
			break;
		case PieceType::King:
			GenerateKingMoves(board, info, square, side, moves);
			break;
		case PieceType::Rook:
		case PieceType::Bishop:
		case PieceType::Queen:
			Side side = sideof(piece);

			SlidingPiecePattern(square, pieceof(piece), [&](Square target, int rayIdx) {
				Piece targetPiece = board[target];

				if (targetPiece == Piece::Empty)
					AddIfLegal(board, info, Move(square, target), moves);
				else if (sideof(targetPiece) == side)
					return false;
				else if (sideof(targetPiece) != side) {
					AddIfLegal(board, info, Move(square, target, board[target]), moves);
					return false;
				}
				return true;
				});
			break;
		}
	}

	size_t Perft(size_t depth, Board& board) {
		if (depth == 0)
			return 1;

		MoveList moves;
		GetAllMoves(board, board.Info(), moves);
		size_t pos = 0;

		for (Move& move : moves) {
			board.PlayMove(move);
			pos += Perft(depth - 1, board);
			board.UnplayMove();
		}
		return pos;
	}
}
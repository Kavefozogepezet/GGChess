#include "Search.h"

#include "BasicTypes.h"
#include "TransposTable.h"
#include "Board.h"

namespace GGChess
{
	Value KingShields(Board& board) {
		const Side sides[2] = { Side::White, Side::Black };
		const Piece myPawn[2] = { Piece::WPawn, Piece::BPawn };
		const SDir dir[2] = { SDir::N, SDir::S };
		const Square base[2] = { Square::a2, Square::a7 };

		Value shield_score = 0;

		for (size_t i = 0; i < 2; i++) {
			Square king = board.King(sides[i]);
			Square shield = base[i];
			Value persp = sides[i] == board.Turn() ? 1 : -1;

			if (rankof(king) > 4) // king stands on the kingside
				shield = shield + 5;

			for (uint8_t delta = 0; delta < 3; delta++, shield = shield + 1) {
				if (board[shield] == myPawn[i])
					shield_score += 10 * persp; // shield stands on rank 2
				else if (board[shield + dir[i]] == myPawn[i])
					shield_score += 5 * persp; // shield on rank 3
			}
		}
		return shield_score;
	}

	Value PawnEval(Board& board, Square square, Side side)
	{
		SDir dir = side == Side::White ? SDir::N : SDir::S;
		Piece
			opposition = PieceType::Pawn | otherside(side),
			friendly = PieceType::Pawn | side;

		bool
			passedFlag = true,
			weakFlag = true,
			opposedFlag = false;

		Value eval = 0;

		bool fileA = fileof(square) == 0;
		bool fileH = fileof(square) == 7;
		uint8_t maxStep = side == Side::White ? 8 - rankof(square) : rankof(square) + 1;

		Square target = square + dir;
		for (uint8_t step = 1; step < maxStep; step++) {
			if (board[target] == opposition) {
				opposedFlag = true;
				passedFlag = false;
			}
			else if (board[target] == friendly) {
				eval -= 20; // doubled pawn
				passedFlag = false;
			}

			if (!fileA && board[target + SDir::W] == opposition) // can be attacked from the side
				passedFlag = false;

			if (!fileH && board[target + SDir::E] == opposition)
				passedFlag = false;

			target = target + dir;
		}

		target = square - dir;
		for (uint8_t step = 1; step < 9 - maxStep; step++) {
			if (!fileA && board[target + SDir::W] == friendly)
				weakFlag = false;

			if (!fileH && board[target + SDir::E] == friendly)
				weakFlag = false;

			if (!weakFlag)
				break;

			target = target - dir;
		}

		Square pstSquare = side == Side::White ? square : flipside(square);

		if (passedFlag)
			eval += PSTables::passedPawn[rankof(square)];

		if (weakFlag) {
			eval += PSTables::weakPawn[fileof(square)];
			if (!opposedFlag)
				eval -= 4;
		}

		return 0;
	}

	Value Evaluate(Board& board, const PosInfo& pos)
	{
		SimpleTTEntry ttentry;
		if (tpostable.ett_probe(board.Key(), ttentry))
			return ttentry.eval;

		Value
			mgScore = 0,
			egScore = 0,
			materialScore = 0,
			pawnScore = 0;

		int phase = 0;

		SimpleTTEntry entry;
		bool ptt_hit = tpostable.ptt_probe(board.PKey(), entry);
		if (ptt_hit)
			pawnScore = entry.eval;

		for (int i = 0; i < 64; i++) {
			Piece p = board[(Square)i];
			PieceType pt = pieceof(p);

			if (p == Piece::Empty)
				continue;

			if (!ptt_hit && pt == PieceType::Pawn)
				PawnEval(board, Square(i), sideof(p));

			Side ps = sideof(p);
			Value persp = ps == board.Turn() ? 1 : -1;

			//game phase
			phase += phaseInc[(int)pt];

			// piece value sum
			Value pieceValue = valueof(pt);
			materialScore += pieceValue * persp;

			// piece square table
			Square square = ps == Side::White ? flipside(Square(i)) : Square(i);
			mgScore += PSTables::middlegame[(int)pt][square] * persp;
			egScore += PSTables::endgame[(int)pt][square] * persp;
		}

		// phase blend
		phase = std::min(phase, 24);
		Value phaseScore = (mgScore * phase + egScore * (24 - phase)) / 24;

		Value finalScore = materialScore + phaseScore;

		finalScore += KingShields(board);

		tpostable.ett_save(board.Key(), finalScore);
		return finalScore;
	}
}

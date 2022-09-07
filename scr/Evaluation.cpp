#include "Search.h"

#include "BasicTypes.h"
#include "TransposTable.h"
#include "Board.h"
#include "MovePatterns.h"

namespace GGChess
{
	struct EvalData
	{
		Value endgame, middlegame, material, pawn;
		int nearKing, phase, bishops, knights, rooks;

		EvalData() :
			endgame(0), middlegame(0), material(0), pawn(0),
			nearKing(0), phase(0), bishops(0), knights(0), rooks(0)
		{}
	};

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

	void PawnEval(Board& board, const PosInfo& info, EvalData& score, Square square, Side side)
	{
		SDir dir = side == Side::White ? SDir::N : SDir::S;
		Piece
			opposition = PieceType::Pawn | otherside(side),
			friendly = PieceType::Pawn | side;

		bool
			passedFlag = true,
			weakFlag = true,
			opposedFlag = false;

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
				score.pawn -= 20; // doubled pawn
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
			score.pawn += PSTables::passedPawn[rankof(square)];

		if (weakFlag) {
			score.pawn += PSTables::weakPawn[fileof(square)];
			if (!opposedFlag)
				score.pawn -= 4;
		}
	}

	void KnightEval(Board& board, const PosInfo& info, EvalData& score, Square square, Piece piece)
	{
		uint8_t
			mobility = 0,
			nearKing = 0;

		Side side = sideof(piece);
		size_t oIdx = side == Side::White ? 1 : 0; // index of opposition

		KnightPattern(square, [&](Square target) {
			if (sideof(board[target]) != side) {
				if (!info.pAttackBoard[oIdx].Get(target))
					mobility++;
				if (isnear(target, board.King(otherside(side))))
					nearKing++;
			}
			return true;
			});

		score.middlegame += 4 * (mobility - 4);
		score.endgame += 4 * (mobility - 4);
		score.nearKing += nearKing * 2;
	}

	void SlidingPieceEval(Board& board, const PosInfo& info, EvalData& score, Square square, Piece piece)
	{
		const Value
			mgMob[PIECE_COUNT] = { 0, 0, 1, 3, 4, 2, 0 },
			egMob[PIECE_COUNT] = { 0, 0, 2, 3, 4, 4, 0 };

		uint8_t nkValue[PIECE_COUNT] = { 0, 0, 4, 2, 2, 3, 0 };

		uint8_t
			mobility = 0,
			nearKing = 0;

		Side side = sideof(piece);
		PieceType pt = pieceof(piece);
		size_t oIdx = side == Side::White ? 1 : 0; // index of opposition

		SlidingPiecePattern(square, pt, [&](Square target, int rayIdx) {
			Piece t = board[target];
			if (t == Piece::Empty) {
				if (!info.pAttackBoard[oIdx].Get(target))
					mobility++;
				if (isnear(target, board.King(otherside(side))))
					nearKing++;
			}
			else {
				if (sideof(board[target]) != side) {
					mobility++;
					if (isnear(target, board.King(otherside(side))))
						nearKing++;
				}
				return false;
			}
			return true;
			});

		score.middlegame += mgMob[int(pt)] * mobility;
		score.endgame += egMob[int(pt)] * mobility;
		score.nearKing += nkValue[int(pt)] * nearKing;
	}

	void PieceEval(Board& board, const PosInfo& info, EvalData& score, Square square, Piece piece)
	{
		switch (pieceof(piece)) {
		case PieceType::Knight:
			KnightEval(board, info, score, square, piece);
			break;
		case PieceType::Bishop:
		case PieceType::Rook:
		case PieceType::Queen:
			SlidingPieceEval(board, info, score, square, piece);
			break;
		}
	}

	Value Evaluate(Board& board, const PosInfo& info)
	{
		SimpleTTEntry ttentry;
		if (tpostable.ett_probe(board.Key(), ttentry))
			return ttentry.eval;

		EvalData score;

		SimpleTTEntry entry;
		bool ptt_hit = tpostable.ptt_probe(board.PKey(), entry);
		if (ptt_hit)
			score.pawn = entry.eval;

		for (int i = 0; i < 64; i++) {
			Piece p = board[(Square)i];
			PieceType pt = pieceof(p);

			if (p == Piece::Empty)
				continue;

			if (!ptt_hit && pt == PieceType::Pawn)
				PawnEval(board, info, score, Square(i), sideof(p));
			else
				PieceEval(board, info, score, Square(i), p);

			switch (pt) {
			case PieceType::Bishop: score.bishops++; break;
			case PieceType::Knight: score.knights++; break;
			case PieceType::Rook: score.rooks++; break;
			}

			Side ps = sideof(p);
			Value persp = ps == board.Turn() ? 1 : -1;

			//game phase
			score.phase += phaseInc[(int)pt];

			// piece value sum
			Value pieceValue = valueof(pt);
			score.material += pieceValue * persp;

			// piece square table
			Square square = ps == Side::White ? flipside(Square(i)) : Square(i);
			score.middlegame += PSTables::middlegame[(int)pt][square] * persp;
			score.endgame += PSTables::endgame[(int)pt][square] * persp;
		}			

		// phase blend
		score.phase = std::min(score.phase, 24);
		Value phaseScore = (score.middlegame * score.phase + score.endgame * (24 - score.phase)) / 24;

		Value finalScore = score.material + phaseScore + score.pawn;

		// rewards and penalties for pieces
		if (score.bishops > 1)
			finalScore += 30;
		if (score.knights > 1)
			finalScore -= 8;
		if (score.rooks > 1)
			finalScore -= 16;

		finalScore += KingShields(board);

		tpostable.ett_save(board.Key(), finalScore);
		return finalScore;
	}
}

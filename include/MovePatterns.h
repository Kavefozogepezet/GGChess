#pragma once

#include "BasicTypes.h"

namespace GGChess
{
	template<typename Func>
	void KnightPattern(Square square, Func func)
	{
		static SDir const knightMoves[8] = { (SDir)17, (SDir)10, (SDir)-6, (SDir)-15, (SDir)-17, (SDir)-10, (SDir)6, (SDir)15 };

		for (size_t i = 0; i < 8; i++) {
			Square target = square + knightMoves[i];

			if (!validsquare(target))
				continue;

			int8_t
				deltaRank = std::abs(rankof(square) - rankof(target)),
				deltaFile = std::abs(fileof(square) - fileof(target));

			if (deltaRank + deltaFile != 3)
				continue;

			if (!func(target))
				break;
		}
	}

	template<typename Func>
	void SlidingPiecePattern(Square square, PieceType piece, Func func)
	{
		static SDir const directions[8] = { SDir::N, SDir::NE, SDir::E, SDir::SE, SDir::S, SDir::SW, SDir::W, SDir::NW };

		int8_t
			rank = rankof(square),
			file = fileof(square),
			ndis = 7 - rank,
			edis = 7 - file;

		int8_t dist[8] = { ndis, std::min(ndis, edis), edis, std::min(edis, rank), rank, std::min(rank, file), file, std::min(file, ndis) };
		int first = 0, step = 1;

		if (piece == PieceType::Bishop) { first = 1; step = 2; }
		else if (piece == PieceType::Rook) { step = 2; }

		for (int i = first; i < 8; i += step)
		{
			Square target = square + directions[i];

			for (int j = 0; j < dist[i]; j++, target = target + directions[i]) {
				if (!func(target, i)) {
					break;
				}
			}
		}
	}

	template<typename Func>
	void PawnPattern(Square square, Side side, Func func, bool attackOnly = false)
	{
		int8_t baseRank = side == Side::White ? 1 : 6;
		int8_t topRank = side == Side::White ? 7 : 0;

		if (topRank == rankof(square))
			return;

		int8_t file = fileof(square);
		SDir dir = side == Side::White ? SDir::N : SDir::S;

		if (!attackOnly)
			if (func(square + dir, false) && rankof(square) == baseRank)
				func(square + dir + dir, false);

		if (file != 0)
			func(square + dir + SDir::W, true);

		if (file != 7)
			func(square + dir + SDir::E, true);
	}
}

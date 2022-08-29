#pragma once

#include "BasicTypes.h"
#include <cstdint>

namespace GGChess
{
	typedef uint64_t ZobristKey;

	class Board;

	class RandGen
	{
	public:
		RandGen(uint64_t seed) : s(seed) {}
		uint64_t rand();
	private:
		uint64_t s;
	};

	class ZobristHash
	{
	public:
		ZobristHash();

		void calculate(const Board& board);

		void piece(Piece piece, Square square);
		void castle(CastleFlag flags);
		void enPassant(Square square);
		void flipSide();

		ZobristKey key() const;

		static void init();
	private:
		ZobristKey currentKey;
	private:
		static bool initFlag;
		static ZobristKey pieces[2][PIECE_COUNT][BOARD_SQUARE_COUNT];
		static ZobristKey castling[16];
		static ZobristKey ep[BOARD_SIZE];
		static ZobristKey turn;
	};
}

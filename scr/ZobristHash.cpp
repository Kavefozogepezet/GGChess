#include "ZobristHash.h"

#include "Board.h"
#include "BasicTypes.h"

namespace GGChess
{
	bool ZobristHash::initFlag = false;
	ZobristKey ZobristHash::pieces[2][PIECE_COUNT][BOARD_SQUARE_COUNT];
	ZobristKey ZobristHash::castling[16];
	ZobristKey ZobristHash::ep[BOARD_SIZE];
	ZobristKey ZobristHash::turn;


	uint64_t RandGen::rand() {
		s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
		return s * 2685821657736338717LL;
	}

	void ZobristHash::init()
	{
		if (initFlag)
			return;

		RandGen rgen(1070372);

		for (size_t side = 0; side < 2; side++)
			for (size_t pIdx = 0; pIdx < PIECE_COUNT; pIdx++)
				for (size_t sIdx = 0; sIdx < BOARD_SQUARE_COUNT; sIdx++)
					pieces[side][pIdx][sIdx] = rgen.rand();

		for (size_t i = 0; i < 16; i++)
			castling[i] = rgen.rand();

		for (size_t i = 0; i < BOARD_SIZE; i++)
			ep[i] = rgen.rand();

		turn = rgen.rand();
	}

	ZobristHash::ZobristHash() :
		currentKey(0)
	{
		ZobristHash::init();
	}

	void ZobristHash::piece(Piece piece, Square square) {
		size_t
			tIdx = (sideof(piece) == Side::White ? 0 : 1),
			pIdx = (size_t)pieceof(piece),
			sIdx = (size_t)square;

		currentKey ^= pieces[tIdx][pIdx][sIdx];
	}

	void ZobristHash::castle(CastleFlag flags) {
		currentKey ^= castling[flags];
	}

	void ZobristHash::enPassant(Square square) {
		currentKey ^= ep[(size_t)square];
	}

	void ZobristHash::flipSide() {
		currentKey ^= turn;
	}

	void ZobristHash::calculate(const Board& board)
	{
		currentKey = 0;

		for (size_t i = 0; i < BOARD_SQUARE_COUNT; i++)
			if (board[i] != Piece::Empty)
				piece(board[i], (Square)i);

		castle(board.Castling());
		enPassant(board.EPTarget());

		if (board.Turn() == Side::Black)
			flipSide();
	}

	ZobristKey ZobristHash::key() const {
		return currentKey;
	}
}

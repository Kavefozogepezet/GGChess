#include "BasicTypes.h"

#include <algorithm>

namespace GGChess
{
	// SIDE

	inline Side otherside(Side side) {
		return side ^ Side::SideBits;
	}

	inline Side operator ^ (Side lhs, Side rhs) {
		return Side(int8_t(lhs) ^ uint8_t(rhs));
	}

	// PIECE

	inline Piece operator | (PieceType piece_t, Side side) {
		return Piece(uint8_t(piece_t) | uint8_t(side));
	}

	inline PieceType pieceof(Piece piece) {
		return PieceType(uint8_t(piece) & uint8_t(PieceType::PieceBits));
	}

	inline Side sideof(Piece piece) {
		return Side(uint8_t(piece) & uint8_t(Side::SideBits));
	}

	// VALUE

	inline Value valueof(PieceType piece) {
		static const Value values[7] = { 0, 0, 1000, 350, 350, 525, 100 };
		return values[uint8_t(piece)];
	}

	inline Value valueof(Piece piece) {
		return valueof(pieceof(piece));
	}

	// SQUARE

	bool isnear(Square sq1, Square sq2) {
		return std::abs(rankof(sq1) - rankof(sq2)) < 2 && std::abs(fileof(sq1) - fileof(sq2)) < 2;
	}

	inline bool validsquare(Square square) {
		return int8_t(square) >= 0 && int8_t(square) < 64;
	}

	inline Square flipside(Square square) {
		return Square(uint8_t(square) ^ 0b111000);
	}

	inline int8_t rankof(Square square) {
		return uint8_t(square) >> 3;
	}

	inline int8_t fileof(Square square) {
		return uint8_t(square) & 0b111;
	}

	inline Square operator + (Square square, int8_t delta) {
		return Square(int8_t(square) + delta);
	}

	inline Square operator - (Square square, int8_t delta) {
		return Square(int8_t(square) - delta);
	}

	inline Square operator + (Square square, int delta) {
		return Square(int(square) + delta);
	}

	inline Square operator - (Square square, int delta) {
		return Square(int(square) - delta);
	}

	// SDir

	inline Square operator + (Square square, SDir dir) {
		return Square(int8_t(square) + int8_t(dir));
	}

	// CASTLE FLAG

	inline CastleFlag operator ~ (CastleFlag flag) {
		return CastleFlag(~ uint8_t(flag));
	}

	inline CastleFlag& operator &= (CastleFlag& lhs, CastleFlag rhs) {
		return lhs = lhs & rhs;
	}

	inline CastleFlag operator & (CastleFlag lhs, CastleFlag rhs) {
		return CastleFlag(uint8_t(lhs) & uint8_t(rhs));
	}

	inline CastleFlag& operator |= (CastleFlag& lhs, CastleFlag rhs) {
		return lhs = lhs | rhs;
	}

	inline CastleFlag operator | (CastleFlag& lhs, CastleFlag rhs) {
		return CastleFlag(uint8_t(lhs) | uint8_t(rhs));
	}

	// BIT BOARD
	/*
	const BitBoard BitBoard::AFile = BitBoard(0x0101010101010101ULL);
	const BitBoard BitBoard::HFile = BitBoard(0x0101010101010101ULL << 7);
	*/
	inline void BitBoard::Set(uint64_t mask, bool state) {
		if (state)
			bits |= mask;
		else
			bits &= ~(mask);
	}

	inline void BitBoard::Set(Square square, bool state) {
		Set(uint64_t(1LL << uint64_t(square)), state);
	}

	inline long long BitBoard::Get(uint64_t mask) const {
		return bits & mask;
	}

	inline bool BitBoard::Get(Square square) const {
		return Get(uint64_t(1LL << uint64_t(square)));
	}

	inline BitBoard BitBoard::pawnAttack(Side side) const {
		return side == Side::White ?
			(bits & ~fileA) << 7 | (bits & ~fileH) << 9 :
			(bits & ~fileA) >> 9 | (bits & ~fileH) >> 7;
	}

	inline BitBoard& BitBoard::operator |= (const BitBoard& other) {
		bits |= other.bits;
		return *this;
	}

	inline BitBoard BitBoard::operator | (const BitBoard& other) const {
		return bits | other.bits;
	}
}

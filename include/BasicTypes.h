#pragma once

#include <stdint.h>

namespace GGChess
{
    // constants to eliminate magic numbers
    static const size_t PIECE_COUNT = 7; // because None == 0
    static const size_t BOARD_SIZE = 8;
    static const size_t BOARD_SQUARE_COUNT = BOARD_SIZE * BOARD_SIZE;

    static const size_t MAX_MOVES = 256;
    static const size_t MAX_DEPTH = 64;
    static const size_t TABLE_SIZE = 1048576;

    enum class Side : uint8_t
    {
        NoSide  = 0,
        White   = 0b01000,
        Black   = 0b10000,

        SideBits = 0b11000 // Bits representing the side in a piece
    };

    inline Side otherside(Side side);
    inline Side operator ^ (Side lhs, Side rhs);
    
    enum class PieceType : uint8_t
    {
        None = 0,
        King,
        Queen,
        Bishop,
        Knight,
        Rook,
        Pawn,

        PieceBits = 0b111 // Bits representing the piece type in a piece
    };

    enum class Piece : uint8_t
    {
        Empty = 0,

        WKing = 9, WQueen = 10, WBishop = 11, WKnight = 12, WRook = 13, WPawn = 14,
        BKing = 17, BQueen = 18, BBishop = 19, BKnight = 20, BRook = 21, BPawn = 22
    };

    inline Piece operator | (PieceType piece_t, Side side);
    inline PieceType pieceof(Piece piece);
    inline Side sideof(Piece piece);

    typedef int Value;

    inline Value valueof(PieceType piece);
    inline Value valueof(Piece piece);

    enum Square : int8_t
    {
        a1, b1, c1, d1, e1, f1, g1, h1,
        a2, b2, c2, d2, e2, f2, g2, h2,
        a3, b3, c3, d3, e3, f3, g3, h3,
        a4, b4, c4, d4, e4, f4, g4, h4,
        a5, b5, c5, d5, e5, f5, g5, h5,
        a6, b6, c6, d6, e6, f6, g6, h6,
        a7, b7, c7, d7, e7, f7, g7, h7,
        a8, b8, c8, d8, e8, f8, g8, h8,
        InvalidSquare = 127
    };

    inline bool validsquare(Square square);
    inline Square flipside(Square square);

    inline int8_t rankof(Square square);
    inline int8_t fileof(Square square);

    inline Square operator + (Square square, int8_t delta);
    inline Square operator - (Square square, int8_t delta);

    inline Square operator + (Square square, int delta);
    inline Square operator - (Square square, int delta);

    // Add this to a square to advance in a given direction
    enum SDir : int8_t
    {
        N = 8,
        NE = 9,
        E = 1,
        SE = -7,
        S = -8,
        SW = -9,
        W = -1,
        NW = 7
    };
    
    inline Square operator + (Square square, SDir dir);

    enum CastleFlag : uint8_t {
        WhiteKingside   = 0b0001,
        WhiteQueenside  = 0b0010,
        BlackKingside   = 0b0100,
        BlackQueenside  = 0b1000,

        WhiteCastle     = 0b0011,
        BlackCastle     = 0b1100
    };

    inline CastleFlag operator ~ (CastleFlag flag);

    inline CastleFlag& operator &= (CastleFlag& lhs, CastleFlag rhs);
    inline CastleFlag operator & (CastleFlag lhs, CastleFlag rhs);

    inline CastleFlag& operator |= (CastleFlag& lhs, CastleFlag rhs);
    inline CastleFlag operator | (CastleFlag& lhs, CastleFlag rhs);
    
    class Board;

    // representing a move
    struct Move
    {
        enum Flags : uint8_t {
            Basic       = 0,
            EnPassant   = 0b0000001,
            Castle      = 0b0000010,
            DoublePush  = 0b0000100,
            PromoteQ    = 0b0001000,
            PromoteR    = 0b0010000,
            PromoteN    = 0b0100000,
            PromoteB    = 0b1000000,

            Promotion   = 0b1111000 // All promotion flags set to test if move is a promotion
        };

        Square origin;
        Square target;
        Piece captured; // the piece that was originally on the target square, if e.p. then Empty
        Flags flags;

        Move() :
            origin(Square::InvalidSquare), target(Square::InvalidSquare), captured(Piece::Empty), flags(Flags::Basic)
        {}

        Move(Square origin, Square target, Piece captured = Piece::Empty, Flags flags = Flags::Basic) :
            origin(origin), target(target), captured(captured), flags(flags)
        {}

        Move(Square origin, Square target, Flags flags) :
            origin(origin), target(target), captured(Piece::Empty), flags(flags)
        {}
    };

    struct BitBoard
    {
        //static const BitBoard AFile, HFile;

        uint64_t bits;

        BitBoard() : bits(0) {}
        BitBoard(uint64_t bitboard) : bits(bitboard) {}

        inline void Set(uint64_t mask, bool state);
        inline void Set(Square square, bool state);

        inline long long Get(uint64_t mask) const;
        inline bool Get(Square square) const;

        inline BitBoard& operator |= (const BitBoard& other);
        inline BitBoard operator | (const BitBoard& other) const;
    };
}

#include "BasicTypes.inl"

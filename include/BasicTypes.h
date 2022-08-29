#pragma once

namespace GGChess
{
    // constants to eliminate magic numbers
    static const size_t PIECE_COUNT = 7; // because None == 0
    static const size_t BOARD_SIZE = 8;
    static const size_t BOARD_SQUARE_COUNT = BOARD_SIZE * BOARD_SIZE;

    static const size_t TABLE_SIZE = 1048576;

    enum class Side : unsigned char
    {
        White = 8,
        Black = 16,

        SideBits = 24
    };

    inline Side OtherSide(Side side) {
        return side == Side::White ? Side::Black : Side::White;
    }

    enum class Piece : unsigned char
    {
        None = 0,
        King,
        Queen,
        Bishop,
        Knight,
        Rook,
        Pawn
    };

    inline Piece operator | (Piece piece, Side side) {
        return (Piece)((unsigned char)piece | (unsigned char)side);
    }

    inline Piece pieceof(Piece piece) {
        return (Piece)((unsigned char)piece & ~(unsigned char)Side::SideBits);
    }

    inline Side sideof(Piece piece) {
        return (Side)((unsigned char)piece & (unsigned char)Side::SideBits);
    }

    enum Square : char
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

    inline bool validSquare(Square square) {
        return (char)square >= 0 && (char)square < 64;
    }

    inline int rankof(Square square) {
        return (char)square / 8;
    }

    inline int fileof(Square square) {
        return (char)square % 8;
    }

    inline Square operator + (Square square, char delta) {
        return (Square)((char)square + delta);
    }

    inline Square operator - (Square square, char delta) {
        return (Square)((char)square - delta);
    }

    inline Square operator + (Square square, int delta) {
        return (Square)((int)square + delta);
    }

    inline Square operator - (Square square, int delta) {
        return (Square)((int)square - delta);
    }

    // Add this to a square to advance in a given direction
    enum SDir : char
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
    
    inline Square operator + (Square square, SDir dir) {
        return (Square)((char)square + (char)dir);
    }

    enum CastleFlag : unsigned char {
        WhiteKingside = 1,
        WhiteQueenside = 2,
        BlackKingside = 4,
        BlackQueenside = 8
    };
    
    class Board;

    // representing a move
    struct Move
    {
        enum Flags : char {
            Basic = 0,
            EnPassant = 1,
            Castle = 2,
            DoublePush = 4,
            PromoteQ = 8,
            PromoteR = 16,
            PromoteN = 32,
            PromoteB = 64,

            Promotion = 120 // All promotion flags set to test if move is a promotion
        };

        Square origin;
        Square target;
        Piece captured; // the piece that was originally on the target square, if e.p. then None
        Flags flags;

        Move() :
            origin(Square::InvalidSquare), target(Square::InvalidSquare), captured(Piece::None), flags(Flags::Basic)
        {}

        Move(Square origin, Square target, Piece captured = Piece::None, Flags flags = Flags::Basic) :
            origin(origin), target(target), captured(captured), flags(flags)
        {}

        Move(Square origin, Square target, Flags flags) :
            origin(origin), target(target), captured(Piece::None), flags(flags)
        {}
    };

    struct BitBoard
    {
        long long bits;

        BitBoard() : bits(0) {}

        inline void Set(long long mask, bool state) {
            if (state)
                bits |= mask;
            else
                bits &= ~(mask);
        }

        inline void Set(Square square, bool state) {
            Set((long long)(1LL << (long long)square), state);
        }

        inline long long Get(long long mask) const {
            return bits & mask;
        }

        inline bool Get(Square square) const {
            return Get((long long)(1LL << (long long)square));
        }
    };
}

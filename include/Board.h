#pragma once

#include <set>
#include <list>
#include <vector>
#include <array>

#include "BasicTypes.h"
#include "ZobristHash.h"
#include "FastArray.h"

namespace GGChess
{
    struct PosInfo
    {
        bool
            check,
            doubleCheck;

        BitBoard
            checkBoard,
            attackBoard,
            pAttackBoard[2], // idx 0 white, idx 1 black
            unifiedPinBoard;

        std::array<BitBoard, 8> pinBoards;

        PosInfo() :
            check(false), doubleCheck(false),
            checkBoard(), attackBoard(), pAttackBoard{},
            unifiedPinBoard(), pinBoards()
        {}
    };

    class Board
    {
    public:
        friend class Fen;

        struct MoveData {
            Move move;
            CastleFlag castle;
        };

    public:
        Board();

        void PlayUnrecorded(const Move& move);
        void PlayMove(const Move& move);

        void UnplayMove();

        const Piece& operator [] (Square square) const;
        const Piece& operator [] (size_t idx) const;

        const Piece& at(Square square) const;
        const Piece& at(size_t idx) const;

        int Ply() const;
        Side Turn() const;
        Square King(Side side) const;
        const std::array<Piece, 64>& array() const;
        Square EPTarget() const;
        PosInfo Info() const;
        ZobristKey Key() const;
        ZobristKey PKey() const;
        CastleFlag Castling() const;

        bool CanCastle(CastleFlag flag) const;

        void SetThisAsStart();
    private:
        std::array<Piece, BOARD_SQUARE_COUNT> board;

        ZobristHash hash;
        ZobristHash phash;

        Square whiteKing;
        Square blackKing;

        Side turn;

        Square ep_start;
        Square ep_target;

        CastleFlag castling;

        int ply;
    public:
        FastArray<MoveData, MAX_DEPTH> moveRecord;
    private:
        void PlacePiece(Square square, Piece piece);
        void RemovePiece(Square square);

        void MovePiece(Square origin, Square target);
    };
}

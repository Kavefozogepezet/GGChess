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
            unifiedPinBoard;

        std::array<BitBoard, 8> pinBoards;

        PosInfo() :
            check(false), doubleCheck(false),
            checkBoard(), attackBoard(),
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

        int GetHalfMoves() const;
        Side GetTurn() const;
        Square GetKing(Side side) const;
        const std::array<Piece, 64>& GetBoard() const;
        Square GetEnPassantTarget() const;
        PosInfo GetPosInfo() const;
        ZobristKey GetPosKey() const;
        CastleFlag GetCastleState() const;

        bool CanCastle(CastleFlag flag) const;

        void SetThisAsStart();
    private:
        std::array<Piece, BOARD_SQUARE_COUNT> board;

        ZobristHash myHash;

        Square whiteKing;
        Square blackKing;

        Side turn;

        Square ep_start;
        Square ep_target;

        CastleFlag castling;

        int halfmove_count;
    public:
        FastArray<MoveData, MAX_DEPTH> moveRecord;
    private:
        void PlacePiece(Square square, Piece piece);
        void RemovePiece(Square square);

        void MovePiece(Square origin, Square target);
    };
}

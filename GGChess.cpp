// UnicodeChess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include<stdio.h>
#include <fcntl.h>
#include <io.h>

#include "BasicTypes.h"
#include "Board.h"
#include "IO.h"
#include "Fen.h"
#include "MoveGenerator.h"
#include "ThreadPool.h"
#include "TransposTable.h"

#include "UCIHandler.h"

using namespace GGChess;

int SearchHelper(int depth, Board& board) {
    if (depth == 0)
        return 1;

    MoveList moves;
    GetAllMoves(board, board.GetPosInfo(), moves);
    int pos = 0;
    /*
    if (board.moveList.back().move.target == Square::b5) {
        std::cout << "\nb5b7 moves:\n";
        for (Move& move : moves)
            std::cout << std::toString(move) << "\n";
        std::cout << std::endl;
    }
    */
    for (Move& move : moves) {
        board.PlayMove(move);
        pos += SearchHelper(depth - 1, board);
        board.UnplayMove();
    }
    return pos;
}

struct PerftThread {
    Board board;
    int depth;

    PerftThread(const Board& board, int depth) :
        board(board), depth(depth)
    {}

    int operator() () {
        return SearchHelper(depth, board);
    }
};

void ShearchBruteForce(int depth, Board& board, ThreadPool& pool)
{
    std::cout << "searching at depth " << depth << " on position:\n" << board << "\n\n";

    if (depth <= 0) {
        std::cout << 1 << std::endl;
        return;
    }

    MoveList moves;
    GetAllMoves(board, board.GetPosInfo(), moves);
    int pos = 0;

    std::vector<std::shared_future<int>> futures;
    futures.reserve(moves.size());

    for (Move& move : moves) {
        board.PlayMove(move);

        futures.push_back(
            pool.submit<PerftThread, int>(PerftThread(board, depth - 1))
        );

        board.UnplayMove();

        /*
        board.PlayMove(move);
        int subpos = SearchHelper(depth - 1, board);
        pos += subpos;
        std::cout << std::toString(move) << ": " << subpos << "\n";
        board.UnplayMove();*/
    }

    size_t idx = 0;
    for (auto& f : futures) {
        f.wait();

        pos += f.get();
        std::cout << std::to_string(moves[idx]) << ": " << f.get() << "\n";

        idx++;
    }

    std::cout << "\nfound positions: " << pos << std::endl;
}

void Play()
{
    Board board;

    while (true) {
        MoveList moves;
        GetAllMoves(board, board.GetPosInfo(), moves);

        std::cout << board << "\n" << std::endl << "pls move: ";
        std::string movestr;
        std::cin >> movestr;

        if (movestr == "back")
            board.UnplayMove();

        for (const Move& move : moves)
            if (std::to_string(move) == movestr)
                board.PlayMove(move);

        system("cls");
    }
}

int main()
{
    /*
    ThreadPool pool;

    while (true) {
        Board board;
        std::string fen;
        int depth;

        std::cout << "fen pls: ";
        std::getline(std::cin, fen);

        std::cout << "depth pls: ";
        std::cin >> depth;

        Fen::Set(board, fen);
        ShearchBruteForce(depth, board, pool);
        std::cin.get();
    }*/
    //Play();
    GGChess::UCIMain();
    
    /*
    BitBoard b;
    b.Set(Square::c5, true);
    std::cout << b << std::endl;*/
}

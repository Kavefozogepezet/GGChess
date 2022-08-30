#include "UCIHandler.h"

#include <iostream>
#include <string>
#include <sstream>
#include <list>

#include "Board.h"
#include "Fen.h"
#include "IO.h"
#include "MoveGenerator.h"
#include "Evaluation.h"
#include "ThreadPool.h"

namespace GGChess
{
	static const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	static ThreadPool threadPool;

	static Board internalBoard;

	static void PrintEngineData()
	{
		std::cout << "id name GGChess\nid author Kavefozogepezet\n\nuciok" << std::endl;
	}

	static void ReadPosition(std::stringstream& stream)
	{
		std::string temp;
		stream >> temp;
		if (temp == "startpos")
			Fen::Set(internalBoard, startpos);
		else if (temp == "fen")
			Fen::Set(internalBoard, stream);

		stream >> temp;
		if (temp == "moves") {
			Square s1, s2;
			char c = 0;
			Move::Flags flags;

			while (stream >> s1 >> s2) {
				if (stream >> std::noskipws >> c >> std::skipws) {
					switch (c) {
					case 'q': flags = Move::Flags::PromoteQ; break;
					case 'r': flags = Move::Flags::PromoteR; break;
					case 'n': flags = Move::Flags::PromoteN; break;
					case 'b': flags = Move::Flags::PromoteB; break;
					default: stream.putback(c); c = 0; break;
					}
				}

				MoveList moves;
				GetMoves(internalBoard, internalBoard.GetPosInfo(), s1, moves);

				for (const Move& move : moves) {
					if (move.origin == s1 && move.target == s2) {
						if (c) {
							if (move.flags == flags)
								internalBoard.PlayUnrecorded(move);
						}
						else {
							internalBoard.PlayUnrecorded(move);
						}
					}
				}
			}
		}
		internalBoard.SetThisAsStart();
		std::cout << "info read position" << std::endl;
	}

	static void ExecuteGo(std::stringstream& stream)
	{
		Move best = FindBestMove(threadPool, internalBoard, 6);

		std::cout << "bestmove " << best << std::endl;
		std::cout << "info evaluated" << std::endl;
		//internalBoard.PlayMove(*front);
	}

	int SearchHelper(int depth, Board& board) {
		if (depth == 0)
			return 1;

		MoveList moves;
		GetAllMoves(internalBoard, internalBoard.GetPosInfo(), moves);
		int pos = 0;

		for (Move& move : moves) {
			ZobristKey zkey = internalBoard.GetPosKey();

			board.PlayMove(move);
			pos += SearchHelper(depth - 1, board);
			board.UnplayMove();
		}
		return pos;
	}

	void ShearchBruteForce(int depth, Board& board)
	{
		std::cout << "searching at depth " << depth << " on position:\n" << board << "\n\n";

		if (depth <= 0) {
			std::cout << 1 << std::endl;
			return;
		}

		MoveList moves;
		GetAllMoves(internalBoard, internalBoard.GetPosInfo(), moves);
		int pos = 0;

		for (Move& move : moves) {
			board.PlayMove(move);
			int subpos = SearchHelper(depth - 1, board);
			pos += subpos;
			std::cout << std::toString(move) << ": " << subpos << "\n";
			board.UnplayMove();
		}
		std::cout << "\nfound positions: " << pos << std::endl;
	}

	static void Perft(std::stringstream& stream)
	{
		int depth;
		stream >> depth;

		ShearchBruteForce(depth, internalBoard);
	}

	static void PrintCaptures() {
		MoveList moves;
		GetAllCaptures(internalBoard, internalBoard.GetPosInfo(), moves);
		for (Move& move : moves) {
			std::cout << std::toString(move) << " " << BadCapture(internalBoard, move) << std::endl;
		}
	}

	static void ExecuteCommand(const std::string& line)
	{
		std::stringstream stream(line);
		std::string first;
		stream >> first;

		if (first == "uci")
			PrintEngineData();
		else if (first == "isready")
			std::cout << "readyok" << std::endl;
		else if (first == "ucinewgame")
			internalBoard = Board();
		else if (first == "d")
			std::cout << internalBoard << std::endl;
		else if (first == "position")
			ReadPosition(stream);
		else if (first == "go")
			ExecuteGo(stream);
		else if (first == "perft")
			Perft(stream);
		else if (first == "eval")
			std::cout << "Position evaluation: " << EvaluatePos(internalBoard, internalBoard.GetPosInfo()) << std::endl;
		else if (first == "captures")
			PrintCaptures();
	}

	void UCIMain()
	{
		while (true)
		{
			std::string input;
			std::getline(std::cin, input);
			ExecuteCommand(input);
		}
	}
}
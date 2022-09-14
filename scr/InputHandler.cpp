#include "InputHandler.h"

#include <iostream>
#include <string>
#include <sstream>
#include <list>

#include "Board.h"
#include "Fen.h"
#include "IO.h"
#include "MoveGenerator.h"
#include "Search.h"
#include "ThreadPool.h"

namespace GGChess
{
	static const char* startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	static Board internalBoard;

	static void PrintEngineData()
	{
		UCI_ID(GGChess, Kavefozogepezet);
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
				GetMoves(internalBoard, internalBoard.Info(), s1, moves);

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
	}

	static void ExecuteGo(std::stringstream& stream)
	{
		Limits limits;
		std::string limit_name;

		while (stream) {
			stream >> limit_name;

			if (limit_name == "wtime") stream >> limits.wtime;
			else if (limit_name == "btime") stream >> limits.btime;
			else if (limit_name == "winc") stream >> limits.winc;
			else if (limit_name == "binc") stream >> limits.binc;
		}

		Move best = Search(internalBoard, limits);
		UCI_BESTMOVE(best);
	}

	static void ExecutePerft(std::stringstream& stream)
	{
		size_t depth;
		stream >> depth;

		std::cout << "searching at depth " << depth << " on position:\n" << internalBoard << "\n\n";

		if (depth <= 0) {
			std::cout << 1 << std::endl;
			return;
		}

		MoveList moves;
		GetAllMoves(internalBoard, internalBoard.Info(), moves);
		size_t pos = 0;

		for (Move& move : moves) {
			internalBoard.PlayMove(move);
			size_t subpos = Perft(depth - 1, internalBoard);
			pos += subpos;
			std::cout << move << ": " << subpos << "\n";
			internalBoard.UnplayMove();
		}
		std::cout << "\nfound positions: " << pos << std::endl;
	}

	static void PrintCaptures() {
		MoveList moves;
		GetAllCaptures(internalBoard, internalBoard.Info(), moves);
		for (Move& move : moves) {
			std::cout << std::to_string(move) << " " << BadCapture(internalBoard, move) << std::endl;
		}
	}

	static void ExecutePlay(std::stringstream& stream)
	{
		char side_char;
		stream >> side_char;

		Side mySide = side_char == 'b' ? Side::Black : Side::White;
		bool resigned = false;

		PosInfo info;

		while (true) {
			system("cls");
			std::cout << internalBoard << std::endl;
			info = internalBoard.Info();

			if (internalBoard.Turn() == mySide) {
				Move move = Search(internalBoard, Limits());

				if (move.origin == Square::InvalidSquare)
					break;

				internalBoard.PlayUnrecorded(move);
			}
			else {
				MoveList moves;
				GetAllMoves(internalBoard, internalBoard.Info(), moves);

				if (moves.size() == 0)
					break;

				std::cout << "play a move: ";
				std::string move_in;
				std::cin >> move_in;

				if (move_in == "resign") {
					resigned = true;
					break;
				}

				for (const Move& move : moves)
					if (std::to_string(move) == move_in)
						internalBoard.PlayUnrecorded(move);
			}
		}
		if (!resigned) {
			if (info.check)
				std::cout <<
				"Checkmate, " <<
				std::to_string(otherside(internalBoard.Turn())) <<
				" is victorious" << std::endl;
			else
				std::cout << "Draw" << std::endl;
		}
		else {
			std::cout <<
				std::to_string(otherside(mySide)) <<
				" resigned, " <<
				std::to_string(mySide) <<
				" is victorious" << std::endl;
		}

		std::cin.get();
		system("cls");
	}

	static void ExecuteCommand(const std::string& line)
	{
		std::stringstream stream(line);
		std::string first;
		stream >> first;

		if (first == "uci")
			PrintEngineData();
		else if (first == "isready")
			UCI_READY;
		else if (first == "ucinewgame")
			internalBoard = Board();
		else if (first == "d")
			std::cout << internalBoard << std::endl;
		else if (first == "position")
			ReadPosition(stream);
		else if (first == "go")
			ExecuteGo(stream);
		else if (first == "perft")
			ExecutePerft(stream);
		else if (first == "eval")
			std::cout << "Position evaluation: " << Evaluate(internalBoard, internalBoard.Info()) << std::endl;
		else if (first == "captures")
			PrintCaptures();
		else if (first == "info")
			std::cout << internalBoard.Info().attackBoard << std::endl;
		else if (first == "playme")
			ExecutePlay(stream);
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
	void printSearchData(SearchData& sdata, bool forced)
	{
		static size_t prev_depth = 0;
		static Timer timer;

		if (!forced && prev_depth == sdata.depth && !(timer.elapsed() > 2000))
			return;

		timer.reset();
		prev_depth = sdata.depth;

		UCI_INFO << "depth " << sdata.depth <<
			" score cp " << sdata.best.score <<
			" nodes " << sdata.nodes <<
			" qnodes " << sdata.qnodes <<
			" time " << sdata.timer.elapsed() <<
			" asp_fail " << sdata.aspf <<
			" pv " << sdata.best.myMove << std::endl;
	}
}
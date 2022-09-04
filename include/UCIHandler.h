#pragma once

#include "IO.h"

#define UCI_INFO std::cout << "info "
#define UCI_BESTMOVE(move) std::cout << "bestmove " << move << std::endl
#define UCI_ID(name, author) std::cout << "name " << #name << '\n' << "author " << #author <<std::endl
#define UCI_OK std::cout << "uciok" << std::endl
#define UCI_READY std::cout << "readyok" << std::endl


namespace GGChess
{
	struct SearchData;
	struct RootMove;

	void UCIMain();

	void printSearchData(size_t depth, SearchData& sdata, RootMove& best);
}

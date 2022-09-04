#pragma once

#include "ZobristHash.h"

namespace GGChess
{
	enum class TTFlag {
		Exact, Alpha, Beta
	};

	struct TTEntry {
		ZobristKey key;
		uint8_t depth;
		Value eval;
		TTFlag flag;
		Move best;
	};

	struct SimpleTTEntry {
		ZobristKey key;
		Value eval;
	};

	class TransposTable // TODO Pawn table
	{
	public:
		TransposTable();
		TransposTable(size_t ttSize, size_t pttSize, size_t ettSize);

		~TransposTable();

		void resize(size_t size);
		bool probe(ZobristKey key, uint8_t depth, Value alpha, Value beta, TTEntry& entry);
		void save(ZobristKey key, uint8_t depth, Value eval, TTFlag flag, Move best);
		void prefetch(ZobristKey key);

		/*
		void pawntt_resize(size_t size);
		bool pawntt_probe(ZobristKey key);
		void pawntt_save(ZobristKey key, Value eval);
		*/

		void evaltt_resize(size_t size);
		bool evaltt_probe(ZobristKey key, SimpleTTEntry& entry);
		void evaltt_save(ZobristKey key, Value eval);
	private:
		TTEntry* tt;
		size_t tt_size;

		SimpleTTEntry* ptt;
		size_t ptt_size;

		SimpleTTEntry* ett;
		size_t ett_size;

		size_t resize_tt(void** table, size_t entry_size, size_t size);
	};

	extern TransposTable tpostable;
}

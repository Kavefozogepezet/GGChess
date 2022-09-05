#include "TransposTable.h"

#include <stdlib.h>
#include <algorithm>
#include <xmmintrin.h>

namespace GGChess
{
	TransposTable tpostable;

	TransposTable::TransposTable() :
		tt(nullptr), ptt(nullptr), ett(nullptr)
	{
		resize(0x4000000);
		ptt_resize(0x1000000);
		ett_resize(0x2000000);
	}

	TransposTable::TransposTable(size_t ttSize, size_t pttSize, size_t ettSize) :
		tt(nullptr), ptt(nullptr), ett(nullptr)
	{
		resize(ttSize);
		ptt_resize(pttSize);
		ett_resize(ettSize);
	}

	TransposTable::~TransposTable()
	{
		if (tt) free(tt);
		if (ptt) free(ptt);
		if (ett) free(ett);
	}

	void TransposTable::resize(size_t size) {
		tt_size = resize_tt((void**)(&tt), sizeof(TTEntry), size);
	}

	bool TransposTable::probe(ZobristKey key, uint8_t depth, Value alpha, Value beta, TTEntry& entry)
	{
		if (tt_size) {
			entry = tt[key & tt_size];

			if (entry.key == key && entry.depth >= depth) {
				switch (entry.flag) {
				case TTFlag::Exact:
					return true;
				case TTFlag::Alpha:
					entry.eval = std::max(entry.eval, alpha);
					return true;
				case TTFlag::Beta:
					entry.eval = std::min(entry.eval, beta);
					return true;
				}
			}
		}
		return false;
	}

	void TransposTable::save(ZobristKey key, uint8_t depth, Value eval, TTFlag flag, Move best)
	{
		if (!tt_size) return;

		TTEntry& entry = tt[key & tt_size];

		if (key == entry.key && entry.depth > depth)
			return;

		entry.key = key;
		entry.depth = depth;
		entry.eval = eval;
		entry.flag = flag;
		entry.best = best;
	}

	void TransposTable::prefetch(ZobristKey key) {
		_mm_prefetch((char*)&tt[key & tt_size], _MM_HINT_NTA);
	}

	void TransposTable::ptt_resize(size_t size) {
		tt_size = resize_tt((void**)(&ptt), sizeof(SimpleTTEntry), size);
	}

	bool TransposTable::ptt_probe(ZobristKey key, SimpleTTEntry& entry) {
		if (!ptt_size)
			return false;

		entry = ptt[key & ptt_size];

		if (entry.key != key)
			return false;

		return true;
	}

	void TransposTable::ptt_save(ZobristKey key, Value eval) {
		if (!ptt_size)
			return;

		SimpleTTEntry& entry = ptt[key & ptt_size];

		entry.key = key;
		entry.eval = eval;
	}

	void TransposTable::ett_resize(size_t size) {
		ett_size = resize_tt((void**)(&ett), sizeof(SimpleTTEntry), size);
	}

	bool TransposTable::ett_probe(ZobristKey key, SimpleTTEntry& entry)
	{
		if (!ett_size)
			return false;

		entry = ett[key & ett_size];

		if (entry.key == key)
			return true;

		return false;
	}

	void TransposTable::ett_save(ZobristKey key, Value eval)
	{
		if (!ett_size)
			return;

		SimpleTTEntry& entry = ett[key & ett_size];

		entry.key = key;
		entry.eval = eval;
	}

	size_t TransposTable::resize_tt(void** table, size_t entry_size, size_t size)
	{
		if (table)
			free(*table);

		if (size & (size - 1)) {
			size--;
			for (int i = 1; i < 32; i = i * 2)
				size |= size >> i;
			size++;
			size >>= 1;
		}

		if (size < 16) {
			return 0;
		}

		size_t table_size = (size / entry_size) - 1;
		*table = malloc(size);

		return table_size;
	}
}

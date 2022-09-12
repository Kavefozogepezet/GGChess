#pragma once

#include <stdexcept>

template<typename Type, size_t max_size>
class FastArray
{
public:
	FastArray() :
		items((Type*)malloc(sizeof(Type) * max_size)), last(items), count(0)
	{}

	FastArray& operator = (const FastArray& other) {
		for (size_t i = 0; i < max_size; i++) {
			items[i] = other.items[i];
		}
		return *this;
	}

	~FastArray() {
		free(items);
	}

	size_t size() {
		return count;
	}

	size_t capacity() {
		return max_size;
	}

	void clear() {
		count = 0;
		last = items;
	}

	Type* begin() {
		return items;
	}

	Type* end() {
		return last;
	}

	void push_back(const Type& item) {
		if (count == max_size)
			throw std::out_of_range("Fast array full");

		count++;
		*last++ = item;
	}

	Type pop_back() {
		if (!count)
			throw std::out_of_range("Fast array empty");

		count--;
		return *--last;
	}

	Type& back() {
		return *(last - 1);
	}

	Type& operator [] (size_t idx) {
		return items[idx];
	}
private:
	Type* items;
	Type* last;
	size_t count;
};

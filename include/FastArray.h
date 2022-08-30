#pragma once

template<typename Type, size_t max_size>
class FastArray
{
public:
	FastArray() :
		items(), last(items), count(0)
	{}

	FastArray(const FastArray& other) { // TODO capacity independent
		for (size_t i = 0; i < max_size; i++) {
			items[i] = other.items[i];
		}
	}

	FastArray& operator = (const FastArray& other) {
		for (size_t i = 0; i < max_size; i++) {
			items[i] = other.items[i];
		}
		return *this;
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
		count++;
		*last++ = item;
	}

	Type pop_back() {
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
	Type items[max_size];
	Type* last;
	size_t count;
};

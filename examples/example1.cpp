#include "mmgen\generator.h"

#include <iostream>


template<typename T>
mmgen::generator<T> take(mmgen::generator<T> generator, size_t count)
{
	return _MGENERATOR(generator = _TAKE_MGENERATOR(generator), count)
	{
		for (auto&& item : *generator) {
			if (count > 0) {
				--count;
				return mmgen::yield_result<T>(std::forward<decltype(item)>(item));
			} else {
				break;
			}
		}
		return mmgen::yield_result<T>();
	};
}

mmgen::generator<int> infrange(int start, int step = 1)
{
	return _MGENERATOR(current = start, step)
	{
		while (true) {
			auto temp = current;
			current += step;
			return  mmgen::yield_result<int>(temp);
		}
		return mmgen::yield_result<int>();
	};
}

int main()
{
	for (auto i : take(infrange(0), 10)) {
		std::cout << i << std::endl;
	}

	return 0;
}
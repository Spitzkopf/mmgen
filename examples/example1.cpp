#include "mmgen\generator.h"

#include <iostream>


template<typename Gen>
mmgen::generator<typename mmgen::gen_value_type<Gen>> take(Gen&& generator, size_t count)
{
	using value_type = typename mmgen::gen_value_type<Gen>;
	return _MGENERATOR(generator = mmgen::gen_lambda_capture(std::forward<Gen>(generator)), count)
	{
		for (auto&& item : *generator) {
			if (count > 0) {
				--count;
				return mmgen::yield_result<value_type>(std::forward<decltype(item)>(item));
			}
			else {
				break;
			}
		}
		return mmgen::yield_result<value_type>();
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
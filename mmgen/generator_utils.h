#pragma once
#include "generator.h"

#include <vector>
#include <tuple>
#include <deque>


namespace mmgen
{
namespace detail
{
template<typename T, size_t N>
struct tuple_repeat
{
	using type = typename decltype(std::tuple_cat(std::declval<std::tuple<T>>(), std::declval<typename tuple_repeat<T, N - 1>::type>()));
};

template<typename T>
struct tuple_repeat<T, 1>
{
	using type = std::tuple<T>;
};

template<typename T>
struct tuple_repeat<T, 0>
{
	using type = std::tuple<>;
};

template<typename Gen>
struct tee_impl_helper
{
	using value_type = typename mmgen::gen_value_type<Gen>;
	using dequeue_collection = std::vector<std::deque<value_type>>;
	using tee_dequeues = typename std::shared_ptr<dequeue_collection>;
	using shared_generator = typename std::shared_ptr<Gen>;


	template<size_t N>
	static typename mmgen::generator<value_type> make_single_tee_generator(shared_generator generator, tee_dequeues dequeues)
	{
		return{
			_MGENERATOR(generator, dequeues) {
			auto& own_queue = dequeues->at(dequeues->size() - N);
			if (own_queue.empty()) {
				auto next_value = generator->next();
				for (auto& queue : *dequeues) {
					queue.emplace_back(next_value);
				}
			}

			auto value = own_queue.front();
			own_queue.pop_front();
			return mmgen::yield_result<value_type>(value);
		}
		};
	}
};

template<size_t N, typename Gen>
struct tee_impl : public tee_impl_helper<Gen>
{
	static typename tuple_repeat<mmgen::generator<value_type>, N>::type tee(Gen generator)
	{
		tee_dequeues dequeues = std::make_shared<dequeue_collection>();
		dequeues->resize(N);
		return tee(std::make_shared<Gen>(std::move(generator)), dequeues);
	}

	static typename tuple_repeat<mmgen::generator<value_type>, N>::type tee(shared_generator generator, tee_dequeues dequeues)
	{
		mmgen::generator<value_type> current = make_single_tee_generator<N>(generator, dequeues);
		return std::tuple_cat(std::make_tuple(std::move(current)), tee_impl<N - 1, Gen>::tee(generator, dequeues));
	}
};

template<typename Gen>
struct tee_impl<1, Gen> : public tee_impl_helper<Gen>
{
	static typename tuple_repeat<mmgen::generator<value_type>, 1>::type tee(Gen generator)
	{
		return std::make_tuple(std::move(generator));
	}

	static typename tuple_repeat<mmgen::generator<value_type>, 1>::type tee(shared_generator generator, tee_dequeues dequeues)
	{
		return std::make_tuple(make_single_tee_generator<1>(generator, dequeues));
	}
};
}//detail

template<size_t N, typename Gen>
typename detail::tuple_repeat<mmgen::generator<typename mmgen::gen_value_type<Gen>>, N>::type tee(Gen generator)
{
	return detail::tee_impl<N, Gen>::tee(std::move(generator));
}
}
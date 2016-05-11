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
}//tee detail

template<size_t N, typename Gen>
typename detail::tuple_repeat<mmgen::generator<typename mmgen::gen_value_type<Gen>>, N>::type tee(Gen generator)
{
	return detail::tee_impl<N, Gen>::tee(std::move(generator));
}

namespace detail
{
template<typename Gen, typename ...Gens>
struct chain_impl : public mmgen::generator_traits<Gen>
{
	static mmgen::generator<value_type> chain(Gen&& gen, Gens&&... gens)
	{
		return chain_impl<Gen, decltype(chain_impl<Gens...>::chain(std::forward<Gens>(gens)...))>::chain(std::forward<Gen>(gen), chain_impl<Gens...>::chain(std::forward<Gens>(gens)...));
	}
};

template<typename Gen>
struct chain_impl<Gen, Gen> : public mmgen::generator_traits<Gen>
{
	static mmgen::generator<value_type> chain(Gen&& gen1, Gen&& gen2)
	{
		return _MGENERATOR(gen1 = mmgen::gen_lambda_capture(std::forward<Gen>(gen1)), gen2 = mmgen::gen_lambda_capture(std::forward<Gen>(gen2))) {
			for (auto&& item : *gen1) {
				return mmgen::yield_result<value_type>(std::forward<decltype(item)>(item));
			}
			for (auto&& item : *gen2) {
				return mmgen::yield_result<value_type>(std::forward<decltype(item)>(item));
			}
			return mmgen::yield_result<value_type>();
		};
	}
};
}//chain detail

template<typename Gen, typename ...Gens>
mmgen::generator<typename mmgen::gen_value_type<Gen>> chain(Gen&& gen, Gens&&... gens)
{
	return detail::chain_impl<Gen, Gens...>::chain(std::forward<Gen>(gen), std::forward<Gens>(gens)...);
}

template<typename Gen>
mmgen::generator<typename mmgen::gen_value_type<Gen>> take(Gen&& generator, size_t count)
{
	using value_type = typename mmgen::gen_value_type<Gen>;
	return _MGENERATOR(generator = mmgen::gen_lambda_capture(std::forward<Gen>(generator)), count)
	{
		if (count > 0) {
			--count;
			return mmgen::yield_result<value_type>{ generator->next() };
		}
		return mmgen::yield_result<value_type>{};
	};
}

template<typename Gen, typename Pred>
mmgen::generator<typename mmgen::gen_value_type<Gen>> take_while(Gen&& generator, Pred&& predicate)
{
	using value_type = typename mmgen::gen_value_type<Gen>;
	return _MGENERATOR(generator = mmgen::gen_lambda_capture(std::forward<Gen>(generator)), predicate = std::forward<Pred>(predicate))
	{
		auto next = generator->next();
		if (predicate(next)) {
			return mmgen::yield_result<value_type>{ next };
		}
		return mmgen::yield_result<value_type>{};
	};
}

template<typename T>
mmgen::generator<T> repeat(T&& value)
{
	return _MGENERATOR(value = std::forward<T>(value)) {
		return mmgen::yield_result<T>{ value };
	};
}

template<typename T>
mmgen::generator<T> repeat(T&& value, size_t times)
{
	return mmgen::take(mmgen::repeat(std::forward<T>(value)), times);
}

template<typename Gen, typename F>
auto map(Gen&& generator, F&& f) -> mmgen::generator<decltype(std::forward<F>(f)(std::declval<typename mmgen::gen_value_type<Gen>>()))>
{
	using result_type = decltype(std::forward<F>(f)(std::declval<typename mmgen::gen_value_type<Gen>>()));
	return _MGENERATOR(generator = mmgen::gen_lambda_capture(std::forward<Gen>(generator)), f = std::forward<F>(f)) {
		for (auto&& item : *generator) {
			return mmgen::yield_result<result_type>{ f(std::forward<decltype(item)>(item)) };
		}
		return mmgen::yield_result<result_type>{};
	};
}

template<typename Gen, typename Pred>
mmgen::generator<typename mmgen::gen_value_type<Gen>> filter(Gen&& generator, Pred&& predicate)
{
	using value_type = typename mmgen::gen_value_type<Gen>;
	return _MGENERATOR(generator = mmgen::gen_lambda_capture(std::forward<Gen>(generator)), predicate = std::forward<Pred>(predicate))
	{
		for (auto&& item : *generator) {
			if (predicate(std::forward<decltype(item)>(item))) {
				return mmgen::yield_result<value_type>{ std::forward<decltype(item)>(item) };
			}
		}
		return mmgen::yield_result<value_type>{};
	};
}

template<typename T>
auto from_iterable(T&& iterable) -> mmgen::generator<typename std::remove_reference<decltype(*std::begin(std::forward<T>(iterable)))>::type>
{
	using iterator_type = decltype(std::begin(std::forward<T>(iterable)));
	using value_type = typename std::remove_reference<decltype(*std::begin(std::forward<T>(iterable)))>::type;
	return _MGENERATOR(iterable = std::forward<T>(iterable), current = mmgen::detail::optional_storage<iterator_type>{}) {
		if (!current.valid()) {
			current = std::begin(iterable);
		}
		if (*current != iterable.end()) {
			auto value = **current;
			++*current;
			return mmgen::yield_result<value_type>{ value };
		}
		return mmgen::yield_result<value_type>{};
	};
}
}
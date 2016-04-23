#pragma once
#include <exception>
#include <utility>
#include <type_traits>
#include <functional>


namespace mmgen
{
namespace detail
{
template<typename T>
using type_storage = typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type;

//forward declaration for generator_function
template<typename T>
class yield_result;

template<typename T>
using generator_function = typename std::function<yield_result<T>()>;

template<typename T>
struct type_helper
{
	using value_type = typename T;
	using reference_type = typename T&;
	using const_reference_type = typename T const&;
	using pointer_type = typename T*;
	using const_pointer_type = typename const T*;
	using rvalue_reference_type = typename T&&;
};
}

struct generation_ended_exception : public std::exception
{
	using std::exception::exception;
};

template<typename T>
class yield_result : public detail::type_helper<T>
{
public:
	//an empty yield result => function has ended
	yield_result() { throw generation_ended_exception(); }

	yield_result(const_reference_type value)
	{
		new (&m_storage) value_type(value);
	}

	yield_result(rvalue_reference_type value)
	{
		new (&m_storage) value_type(std::forward<value_type>(value));
	}

	~yield_result()
	{
		get_ptr()->~T();
	}

	reference_type operator*()
	{
		return *get_ptr();
	}

	const_reference_type operator*() const
	{
		return *get_ptr();
	}

private:
	pointer_type get_ptr()
	{
		return reinterpret_cast<pointer_type>(&m_storage);
	}

	const_pointer_type get_ptr() const
	{
		return reinterpret_cast<const_pointer_type>(&m_storage);
	}

	detail::type_storage<value_type> m_storage;
};

template<typename T>
class generator : public detail::type_helper<T>
{
public:
	template<typename F>
	generator(F&& generator_proc)
		: m_generator(std::forward<F>(generator_proc))
	{}

	generator() = default;

	generator(const generator&) = delete;
	generator& operator=(const generator&) = delete;

	generator(generator&&) = default;
	generator& operator=(generator&&) = default;

	~generator() = default;

private:
	detail::generator_function<value_type> m_generator;
};
}
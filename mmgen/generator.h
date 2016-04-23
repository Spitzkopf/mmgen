#pragma once
#include <exception>
#include <utility>
#include <type_traits>


namespace mmgen
{
namespace detail
{
template<typename T>
using type_storage = typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type;
}

struct generation_ended_exception : public std::exception
{
	using std::exception::exception;
};

template<typename T>
class yield_result
{
public:
	using value_type = T;
	using reference_type = T&;
	using const_reference_type = T const&;
	using pointer_type = T*;
	using const_pointer_type = const T*;
	using rvalue_reference_type = T&&;

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
}
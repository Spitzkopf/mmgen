#pragma once
#include <exception>
#include <utility>
#include <type_traits>
#include <functional>
#include <iterator>


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

template<typename T>
struct optional_storage : public type_helper<T>
{
	optional_storage()
	{
	}

	optional_storage(const_reference_type value)
	{
		new (&m_storage) value_type(value);
		m_valid = true;
	}

	optional_storage(rvalue_reference_type value)
	{
		new (&m_storage) value_type(std::forward<value_type>(value));
		m_valid = true;
	}

	optional_storage(const optional_storage& rhs)
	{
		new (&m_storage) value_type(*rhs);
		m_valid = true;
	}

	optional_storage& operator=(const optional_storage& rhs)
	{
		if (this != &rhs) {
			destroy();
			new (&m_storage) value_type(*rhs);
			m_valid = true;
		}
		return *this;
	}

	optional_storage(optional_storage&& rhs)
		: m_storage{ std::move(rhs.m_storage) }
		, m_valid{ rhs.m_valid }
	{
		rhs.m_valid = false;
	}

	optional_storage& operator=(optional_storage&& rhs)
	{
		if (this != &rhs) {
			destroy();
			m_storage = std::move(rhs.m_storage);
			m_valid = rhs.m_valid;
			rhs.m_valid = false;
		}
		return *this;
	}

	optional_storage& operator=(rvalue_reference_type value)
	{
		*this = optional_storage(std::forward<value_type>(value));
		return *this;
	}

	~optional_storage()
	{
		destroy();
	}

	reference_type operator*()
	{
		return *get_ptr();
	}

	const_reference_type operator*() const
	{
		return *get_ptr();
	}

	pointer_type get_ptr()
	{
		return reinterpret_cast<pointer_type>(&m_storage);
	}

	const_pointer_type get_ptr() const
	{
		return reinterpret_cast<const_pointer_type>(&m_storage);
	}

	void destroy()
	{
		if (m_valid) {
			get_ptr()->~T();
			m_valid = false;
		}
	}

	bool m_valid;
	detail::type_storage<value_type> m_storage;
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
		: m_storage(value)
	{
	}

	yield_result(rvalue_reference_type value)
		: m_storage(std::forward<value_type>(value))
	{}

	~yield_result() = default;

	reference_type operator*()
	{
		return *m_storage;
	}

	const_reference_type operator*() const
	{
		return *m_storage;
	}

private:
	detail::optional_storage<value_type> m_storage;
};

//forward declaration
template<typename T>
class generator;

template<typename T>
class generator_iterator : 
	public std::iterator<std::input_iterator_tag, T>
{
	friend class generator<T>;
private:
	generator_iterator(detail::generator_function<T>* generator)
		: m_generator{ generator }
		, m_exhausted{ false }
	{
		try {
			m_value = **m_generator();
		} catch (const generation_ended_exception&) {
			m_generator = nullptr;
			m_exhausted = true;
		}
	}

	detail::optional_storage<T> m_value;
	bool m_exhausted;
	detail::generator_function<T>* m_generator;
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
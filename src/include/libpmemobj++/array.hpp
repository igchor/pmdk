/*
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Array container for based on std::array
 */

#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <algorithm>

#include "libpmemobj++/detail/common.hpp"
#include "libpmemobj++/persistent_ptr.hpp"
#include "libpmemobj++/pext.hpp"
#include "libpmemobj.h"

namespace pmem
{

namespace detail
{
// adds memory from offset to offset + count to tx

inline void
conditional_add_range_to_tx(const void *offset, std::size_t count)
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		return;

	// 'that' is not in any open pool
	if (!pmemobj_pool_by_ptr(offset))
		return;

	if (pmemobj_tx_add_range_direct(offset, count))
		throw transaction_error("Could not add an object to the"
		" transaction.");
}
}

namespace obj
{

/**
 * pmem::obj::array - persistent container based on std::array
 * diffrences:
 * - methods which return non-const reference to a element add this element
 *   to transaction, these methods are: operator[], at(), front(), back() 
 *   and non-memeber std::get<>
 * - functions mentioned above are not marked as constexpr and noexcept
 *   (as in c++17 standard)
 * - calls to functions which enable acess to all elements (begin(), data())
 *   add entire array to tx and are not marked noexcept
 * - extra function 'range' which adds requested range to transaction and
 *   returns pair of iterators
 * - swap in not marked noexcept
 */
template<typename T, std::size_t N>
struct array
{
	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* iterator;
	typedef const value_type* const_iterator;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	/* Support for zero-sized arrays */
	value_type elems[N ? N : 1];

	/**
	 *  returns n-th element and conditionally
	 *  adds this element to a transaction
	 */
	reference
	at(size_type n)
	{
		if (n >= N)
			throw std::out_of_range("array::at");

		detail::conditional_add_to_tx(elems + n);

		return elems[n];
	}

	constexpr const_reference
	at(size_type n) const
	{
		if (n >= N)
			throw std::out_of_range("array::at");

		return elems[n];
	}

	reference
	operator[](size_type n)
	{
		return at(n);
	}

	constexpr const_reference
	operator[](size_type n) const
	{
		return at(n);
	}

	pointer
	data()
	{
		detail::conditional_add_to_tx(this);
		return elems;
	}

	constexpr const_pointer
	data() const noexcept
	{
		return elems;
	}

	iterator
	begin()
	{
		detail::conditional_add_to_tx(this);
		return elems;
	}

	iterator
	end()
	{
		detail::conditional_add_to_tx(this);
		return elems + size();
	}

	constexpr const_iterator
	begin() const noexcept
	{
		return elems;
	}

	constexpr const_iterator
	cbegin() const noexcept
	{
		return elems;
	}

	constexpr const_iterator
	end() const noexcept
	{
		return elems + size();
	}

	constexpr const_iterator
	cend() const noexcept
	{
		return elems + size();
	}

	reverse_iterator
	rbegin()
	{
		detail::conditional_add_to_tx(this);
		return elems + size();
	}

	reverse_iterator
	rend()
	{
		detail::conditional_add_to_tx(this);
		return elems;
	}

	constexpr const_reverse_iterator
	rbegin() const noexcept
	{
		return elems + size();
	}

	constexpr const_reverse_iterator
	crbegin() const noexcept
	{
		return elems + size();
	}

	constexpr const_reverse_iterator
	rend() const noexcept
	{
		return elems;
	}

	constexpr const_reverse_iterator
	crend() const noexcept
	{
		return elems;
	}

	reference
	front()
	{
		detail::conditional_add_to_tx(&elems[0]);
		return elems[0];
	}

	reference
	back()
	{
		detail::conditional_add_to_tx(&elems[size() - 1]);
		return elems[size() - 1];
	}

	constexpr const_reference
	front() const
	{
		return elems[0];
	}

	constexpr const_reference
	back() const
	{
		return elems[size() - 1];
	}

	/**
	 * Adds requested range to transaction and returns pair of iterators
	 * to access this range
	 *
	 * @param start start index of requested range
	 * @param n number of elements in range
	 * @return pair of iterators - to begin and end of requested range
	 */
	std::pair<iterator, iterator>
	range(size_type start, size_type n)
	{
		if (start + n >= N)
			throw std::out_of_range("array::window");

		detail::conditional_add_range_to_tx(elems + start, n);

		return {elems + start, elems + start + n};
	}

	constexpr const std::pair<const_iterator, const_iterator>
	range(size_type start, size_type n) const
	{
		if (start + n >= N)
			throw std::out_of_range("array::window");

		return {elems + start, elems + start + n};
	}

	constexpr size_type
	size() const
	{
		return N;
	}

	constexpr size_type
	max_size() const
	{
		return N;
	}

	constexpr bool
	empty() const
	{
		return size() != 0;
	}

	void
	fill(const_reference value)
	{
		// call to begin() will add array to a transaction
		std::fill(begin(), size(), value);
	}

	void
	swap(array& other)
	{
		// calls to begin() will add both arrays to a transaction
		std::swap_ranges(begin(), end(), other.begin());
	}

};

template<typename T, std::size_t N>
inline bool
operator==(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template<typename T, std::size_t N>
inline bool
operator!=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs == rhs);
}

template<typename T, std::size_t N>
inline bool
operator<(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return std::lexicographical_compare(lhs.cbegin(), lhs.cend(),
		rhs.cbegin(), rhs.cend());
}

template<typename T, std::size_t N>
inline bool
operator>(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return rhs < lhs;
}

template<typename T, std::size_t N>
inline bool
operator>=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs < rhs);
}

template<typename T, std::size_t N>
inline bool
operator<=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs > rhs);
}

} /* namespace obj */

} /* namespace pmem */

namespace std
{

// Specialized algorithms [6.2.2.2].

template<typename T, size_t N>
inline void
swap(pmem::obj::array<T, N> &lhs, pmem::obj::array<T, N> &rhs)
{
	lhs.swap(rhs);
}

template<size_t I, typename T, size_t N >
T&
get(pmem::obj::array<T, N> &a)
{
	static_assert(I < N,
		"Index out of bounds in std::get<> (pmem::obj::array)");
	return a.at(I);
}

template<size_t I, typename T, size_t N >
T&&
get(pmem::obj::array<T, N> &&a)
{
	static_assert(I < N,
		"Index out of bounds in std::get<> (pmem::obj::array)");
	return move(a.at(I));
}

template<size_t I, typename T, size_t N >
constexpr const T&
get(const pmem::obj::array<T, N> &a) noexcept
{
	static_assert(I < N,
		"Index out of bounds in std::get<> (pmem::obj::array)");
	return a.at(I);
}

template<size_t I, typename T, size_t N >
constexpr const T&&
get(const pmem::obj::array<T, N> &&a) noexcept
{
	static_assert(I < N,
		"Index out of bounds in std::get<> (pmem::obj::array)");
	return move(a.at(I));
}

// Tuple interface to class template array.
template<typename T, size_t N>
class tuple_size<pmem::obj::array<T, N>> :
public std::integral_constant<size_t, N>
{
};

template< size_t I, class T, size_t N >
struct tuple_element<I, pmem::obj::array<T, N>>
{
	using type = T;
};

} /* namespace std */

#endif /* ARRAY_HPP */


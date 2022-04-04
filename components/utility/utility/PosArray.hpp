//
// PosArray.hpp
//
// Created on: Feb 04, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CORE_INC_UTIL_POSARRAY_HPP
#define CORE_INC_UTIL_POSARRAY_HPP

#include <array>
#include <utility>
#include <cassert>

namespace Utility {

template <class T, std::size_t N>
class PosArray : public std::array<T, N> {
private:
	unsigned pos;  ///< Insert position
	using Base = std::array<T, N>;

	using Base::Base;
public:

	using Base::max_size;
	using Base::operator [];
	using Base::operator=;
	using Base::data;
	using Base::begin;
	using Base::cbegin;
	using Base::rend;
	using Base::crend;
	using Base::fill;
	using Base::swap;
	using Base::at;

	PosArray(): Base{}, pos{0}
	{
	}

	PosArray(const PosArray &a): Base{a}, pos{a.pos}
	{
	}

	PosArray(PosArray &&a): Base{a}, pos{a.pos}
	{
		a.pos = 0;
	}

	PosArray &operator=(const PosArray &a)
	{
		static_cast<Base &>(*this) = a;
		pos = a.pos;

		return *this;
	}

	PosArray &operator=(PosArray &&a)
	{
		static_cast<Base &>(*this) = a;
		pos = a.pos;
		a.pos = 0;

		return *this;
	}

	std::size_t size() const
	{
		return pos;
	}

	std::size_t capacity() const
	{
		return N - pos;
	}

	std::size_t max_size() const noexcept
	{
		return N;
	}

	typename Base::iterator end() noexcept
	{
		return data() + pos;
	}

	typename Base::const_iterator cend() const noexcept
	{
		return data() + pos;
	}

	typename Base::reverse_iterator rbegin() noexcept
	{
		return typename Base::reverse_iterator(data() + pos);
	}

	typename Base::const_reverse_iterator crbegin() const noexcept
	{
		return typename Base::const_reverse_iterator(data() + pos);
	}

	typename Base::reference back()
	{
		return at(pos - 1);
	}

	typename Base::const_reference back() const
	{
		return at(pos - 1);
	}

	typename Base::reference push_back(const T &aInstance)
	{
		new(&at(pos)) T{aInstance};
		return at(pos++);
	}

	typename Base::value_type pop_back()
	{
		auto ret = std::move(at(pos - 1));
		at(--pos).~T();

		return ret;
	}

	void setSize(std::size_t aSize)
	{
		assert(aSize <= N);
		pos = aSize;
	}

	void reset()
	{
		while (capacity() != size()) {
			pop_back();
		}
	}
};

}  // namespace Utility

#endif // CORE_INC_UTIL_POSARRAY_HPP

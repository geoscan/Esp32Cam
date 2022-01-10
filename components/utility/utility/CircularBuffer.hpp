//
// CircularBuffer.hpp
//
// Created on: Jan 10, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_CIRCULARBUFFER_HPP
#define UTILITY_UTILITY_CIRCULARBUFFER_HPP


#include <array>
#include <utility>
#include <type_traits>

namespace Utility {

template <unsigned N>
class CircularCounter {
	unsigned base = 0;
	unsigned sz = 0;

public:
	unsigned tail() const  ///< First element to consume
	{
		return base;
	}

	unsigned head() const  ///< Next insert pos.
	{
		return (base + sz) % N;
	}

	unsigned capacity() const
	{
		return N - sz;
	}

	unsigned size() const
	{
		return sz;
	}

	void push(unsigned a = 1)
	{
		assert(capacity());
		sz += a;
	}

	void pop(unsigned a = 1)
	{
		assert(size());
		base = fwd(base, a);
		--sz;
	}

	static unsigned back(unsigned aBase, unsigned a = 1)
	{
		a = a % N;
		return aBase > a ? aBase - a : N - a + aBase - 1;
	}

	static unsigned fwd(unsigned aBase, unsigned a = 1)
	{
		return (aBase + a) % N;
	}
};

template <class TcontiguousContainer, class Tvalue, unsigned N, int Ninc>
struct CircularIterator {
	unsigned pos;
	TcontiguousContainer &arr;

	template <bool F>
	typename std::enable_if<F>::type
	incPos()
	{
		pos = CircularCounter<N>::fwd(pos, 1);
	}

	template <bool F>
	typename std::enable_if<!F>::type
	incPos()
	{
		pos = CircularCounter<N>::back(pos, 1);
	}

	CircularIterator &operator++()
	{
		incPos<(Ninc > 0)>();
		return *this;
	}

	Tvalue &operator*()
	{
		return arr.at(pos);
	}

	Tvalue *operator->()
	{
		return &arr.at(pos);
	}

	bool operator==(const CircularIterator &a)
	{
		return pos == a.pos;
	}

	bool operator!=(const CircularIterator &a)
	{
		return pos != a.pos;
	}
};

template <class T, unsigned N, bool FallowOverwrite = false>
class CircularBuffer : std::array<uint8_t, N * sizeof(T)> {
public:
	using value_type = T;
	using size_type = unsigned;
	using difference_type = std::ptrdiff_t;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;

private:  // Variable members
	CircularCounter<N> cc;

private:
	template <class ...Ta>
	void emplaceAt(size_type aPos, Ta &&...aElem)
	{
		T *pos = &at(aPos);
		new (pos) T{std::forward<Ta>(aElem)...};
	}

	void destructAt(size_type aPos)
	{
		T *pos = &at(aPos);
		(*pos).~T();
	}

	template <bool F>
	typename std::enable_if<F>::type ensureCapacity()
	{
		if (!cc.capacity()) {
			pop_front();
		}
	}

	template <bool F>
	typename std::enable_if<!F>::type ensureCapacity()
	{
		assert(cc.capacity());
	}

public:  // Types
	using ThisType = CircularBuffer<T, N>;

	using iterator = CircularIterator<ThisType, value_type, N, 1>;
	using const_iterator = CircularIterator<const ThisType, const value_type, N, 1>;
	using reverse_iterator = CircularIterator<ThisType, value_type, N, -1>;
	using const_reverse_iterator = CircularIterator<const ThisType, const value_type, N, -1>;

public:
	using Base = std::array<uint8_t, N * sizeof(T)>;
	using Base::data;
	using Base::swap;

	template <class ...Ta>
	CircularBuffer(): Base{{0}}, cc{}
	{
	}

	reference at(size_type pos)
	{
		return *reinterpret_cast<T *>(data() + sizeof(T) * pos);
	}

	const_reference at(size_type pos) const
	{
		return *reinterpret_cast<T *>(data() + sizeof(T) * pos);
	}

	reference back()
	{
		assert(cc.size());
		return at(CircularCounter<N>::back(cc.head(), 1));
	}

	reference front()
	{
		return at(cc.tail());
	}

	const_reference back() const
	{
		assert(cc.size());
		return at(CircularCounter<N>::back(cc.head(), 1));
	}

	const_reference front() const
	{
		return at(cc.tail());
	}

	size_type size() const  ///< Number of elements stored
	{
		return cc.size();
	}

	size_type capacity() const  ///< The amount of free space left
	{
		return cc.capacity();
	}

	bool empty() const
	{
		return cc.size() == 0;
	}

	void push_back(const T& a)
	{
		emplace_back(a);
	}

	void push_back(T &&a)
	{
		emplace_back(a);
	}

	template <class ...Ta>
	void emplace_back(Ta &&...aArgs)
	{
		ensureCapacity<FallowOverwrite>();
		emplaceAt(cc.head(), std::forward<Ta>(aArgs)...);
		cc.push();
	}

	void pop_front()
	{
		assert(cc.size());
		destructAt(cc.tail());
		cc.pop();
	}

	iterator begin()
	{
		return {cc.tail(), *this};
	}

	iterator end()
	{
		return {cc.head(), *this};
	}

	const_iterator cbegin() const
	{
		return {cc.tail(), *this};
	}

	const_iterator cend() const
	{
		return {cc.head(), *this};
	}

	reverse_iterator rbegin()
	{
		return {CircularCounter<N>::back(cc.head(), 1), *this};
	}

	reverse_iterator rend()
	{
		return {CircularCounter<N>::back(cc.tail(), 1), *this};
	}

	const_reverse_iterator crbegin() const
	{
		return {CircularCounter<N>::back(cc.head(), 1), *this};
	}

	const_reverse_iterator crend() const
	{
		return {CircularCounter<N>::back(cc.tail(), 1), *this};
	}
};

}  // namespace Utility

#endif  // UTILITY_UTILITY_CIRCULARBUFFER_HPP

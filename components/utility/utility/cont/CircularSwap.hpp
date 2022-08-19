//
// CircularSwap.hpp
//
// Created on: Feb 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CORE_INC_APP_SWAPCIRCULARBUFFER_HPP
#define CORE_INC_APP_SWAPCIRCULARBUFFER_HPP

#include <array>
#include <stack>
#include "CircularBuffer.hpp"
#include "utility/cont/PosArray.hpp"
#include <algorithm>

namespace Ut {
namespace Cont {

template <class Tbuf, unsigned Nbuffers>
class CircularSwap {  /// Stores a number of stack-allocated buffers organized in a circular manner
public:
	using BufPtrArray = PosArray<Tbuf *, Nbuffers>;
	using ValueType = Tbuf;

private:
	std::array<Tbuf, Nbuffers> buffers;
	std::stack<Tbuf *, BufPtrArray> stack;  ///< Free buffers
	CircularBuffer<Tbuf *, Nbuffers> ring;  ///< Buffers with payload

public:
	CircularSwap(): buffers{}, stack{}, ring{}
	{
		for (auto &buffer : buffers) {
			stack.push(&buffer);
		}
	}

	BufPtrArray popFree(unsigned aN)
	{
		BufPtrArray arrays;

		while (aN-- && countFull()) {
			arrays.push_back(&popFull());
		}

		return arrays;
	}

	BufPtrArray popFull(unsigned aN)
	{
		BufPtrArray arrays;

		while (aN-- && countFull()) {
			arrays.push_back(&popFull());
		}

		return arrays;
	}

	void pushFree(BufPtrArray &aArrays)
	{
		for (auto &a : aArrays) {
			pushFree(*a);
		}
	}

	void pushFull(BufPtrArray &aArrays)
	{
		for (auto &a : aArrays) {
			pushFull(*a);
		}
	}

	Tbuf &popFree() {  ///< Get a free buffer form stack. \pre It must be checked whether one is available.
		auto *ret = stack.top();
		stack.pop();

		return *ret;
	}

	void pushFree(Tbuf &aBuffer)  ///< Return a previously "borrowed" buffer as empty
	{
		stack.push(&aBuffer);
	}

	Tbuf &popFull()  ///< Get a filled buffer from ring buffer. \pre It must be checked whether one is available.
	{
		auto *ret = ring.front();
		ring.pop_front();

		return *ret;
	}

	void pushFull(Tbuf &aBuffer)  ///< Return a previously "borrowed" buffer as filled
	{
		ring.push_back(&aBuffer);
	}

	unsigned countFree() const
	{
		return stack.size();
	}

	unsigned countFull() const
	{
		return ring.size();
	}
};

}  // namespace Cont
}  // namespace Ut

#endif // CORE_INC_APP_SWAPCIRCULARBUFFER_HPP

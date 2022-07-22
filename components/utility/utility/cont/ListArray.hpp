//
// ListArray.hpp
//
// Created on: Jul 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_CONT_LISTARRAY_HPP_)
#define UTILITY_UTILITY_CONT_LISTARRAY_HPP_

#include "utility/PosArray.hpp"
#include <list>

namespace Utility {
namespace Cont {

/// \brief A dynamically expanded list of const-sized arrays.
///
/// \details Unlike std::vector, it does not deallocate previously stored instances, so all the pointers to them remain
/// valid after `push_back`
///
template <class T, unsigned N>
class ListArray : private std::list<Utility::PosArray<T, N>> {
public:
	using value_type = T;
	using size_type = std::size_t;
	using reference = T &;
	using const_reference = const T &;
private:
	using Base = typename std::list<Utility::PosArray<T, N>>;
public:
	ListArray() : std::list<Utility::PosArray<T, N>>{}
	{
	}
public:
	std::size_t size() const
	{
		if (Base::size() == 0) {
			return 0;
		} else {
			return (Base::size() - 1) * N + Base::back().size();
		}
	}
	reference push_back(const_reference a)
	{
		const auto prevSize = size();

		if (prevSize % N == 0) {
			Base::emplace_back();
		}
		return Base::back().push_back(a);
	}

	reference back()
	{
		return Base::back().back();
	}
private:
	using Base::emplace_back;
	using Base::back;
};

}  // namespace Cont
}  // namespace Utility

#endif // UTILITY_UTILITY_CONT_LISTARRAY_HPP_

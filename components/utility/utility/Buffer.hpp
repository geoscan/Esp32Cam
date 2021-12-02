//
// Buffer.hpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_UTILITY_UTILITY_BUFFER_HPP
#define COMPONENTS_UTILITY_UTILITY_BUFFER_HPP

#include <tuple>
#include <type_traits>
#include <cstdint>

namespace Utility {

namespace Impl {

// How an underlying array type would be interpreted

template <typename T>
struct InterpType {
	using Type = T;
};

template <>
struct InterpType<void> {
	using Type = uint8_t;
};

template <>
struct InterpType<const void> {
	using Type = const uint8_t;
};

}  // namespace Impl

template <typename T>
struct Tbuffer : std::tuple<T *, std::size_t> {
	using std::tuple<T *, std::size_t>::tuple;
	using std::tuple<T *, std::size_t>::operator=;

	using Type = T;

	T *data() const;

	std::size_t size() const;

	typename Impl::InterpType<T>::Type &at(std::uint8_t aPos);

	template <typename TypeTo>
	Tbuffer<TypeTo> as();
};

using Buffer = typename ::Utility::Tbuffer<void>;
using ConstBuffer = typename ::Utility::Tbuffer<const void>;

}  // namespace Utility

#include "Buffer.impl"

#endif  // BUFFER_HPP

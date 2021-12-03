//
// Buffer.hpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_UTILITY_UTILITY_BUFFER_HPP
#define COMPONENTS_UTILITY_UTILITY_BUFFER_HPP

#include <tuple>
#include <array>
#include <type_traits>
#include <cstdint>
#include <initializer_list>

namespace Utility {

namespace Impl {

// void and const void should be interpreted as std::uint8_t

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

// Tbuffer

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

template <typename Tto, typename Tfrom>
Tbuffer<Tto> toBuffer(Tfrom *aFromPtr, std::size_t aBufferSize);

///
/// \brief toBuffer Constructs buffer from an instance for which
/// ::data and ::size methods are defined
///
/// \tparam TdataTraitFrom - some instance defining ::data and ::size
/// parameters
///
/// \return
///
template <typename Tto, typename TdataTraitFrom>
Tbuffer<Tto> toBuffer(TdataTraitFrom &&);

// Thold

template <typename T, std::size_t N>
struct Thold : std::array<T, N> {
	using std::array<T, N>::data;
	using std::array<T, N>::operator=;
	using std::array<T, N>::array;

	void setSize(std::size_t);
	std::size_t size() const;

	Tbuffer<T> asBuffer();

private:
	std::size_t mSize = 0;
};

using Buffer = typename ::Utility::Tbuffer<void>;
using ConstBuffer = typename ::Utility::Tbuffer<const void>;

}  // namespace Utility

#include "Buffer.impl"

#endif  // BUFFER_HPP

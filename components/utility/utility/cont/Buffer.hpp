//
// Buffer.hpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_UTILITY_UTILITY_BUFFER_HPP
#define COMPONENTS_UTILITY_UTILITY_BUFFER_HPP

#include "utility/thr/Semaphore.hpp"
#include <asio.hpp>
#include <tuple>
#include <array>
#include <type_traits>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <utility>

namespace Ut {
namespace Cont {

namespace Impl {

// void, const void, and const volatile void should be interpreted as std::uint8_t

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

template <>
struct InterpType<const volatile void> {
	using Type = const uint8_t;
};

}  // namespace Impl

// Tbuffer

template <typename T>
struct Tbuffer : std::tuple<T *, std::size_t> {
	using std::tuple<T *, std::size_t>::tuple;
	using std::tuple<T *, std::size_t>::operator=;

	using Type = T;
	using InterpType = typename Impl::InterpType<T>::Type;  // T, or one of the uint8_t's derivatives, if T is void

	T *data() const;
	std::size_t size() const;
	InterpType &at(std::uint8_t aPos);
	Tbuffer<T> asSlice(std::size_t aOffset) const;  ///< Constructs a slice from a given offset
	Tbuffer<T> asSlice(std::size_t aOffsetBegin, std::size_t aOffsetEnd) const;  /// Constructs a slice satisfying the given range [aOffsetBegin; aOffsetEnd)
	Tbuffer<T> &slice(std::size_t aOffset)
	{
		*this = asSlice(aOffset);

		return *this;
	}
	Tbuffer<T> &slice(std::size_t aOffsetBegin, std::size_t aOffsetEnd)
	{
		*this = asSlice(aOffsetBegin, aOffsetEnd);

		return *this;
	}

	template <typename TypeTo>
	Tbuffer<TypeTo> as();  ///< Reinterpret the buffer for another type. The type's sizeof() is taken into consideration, so size() of the newly constructed instance may differ
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

template <class Tbuf>
asio::mutable_buffer makeAsioMb(Tbuf &&);

template <class Tbuf>
asio::const_buffer makeAsioCb(Tbuf &&);

using Buffer = typename ::Ut::Cont::Tbuffer<void>;
using ConstBuffer = typename ::Ut::Cont::Tbuffer<const void>;

}  // namespace Cont
}  // namespace Ut

#include "Buffer.impl"

#endif  // BUFFER_HPP
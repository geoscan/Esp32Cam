//
// EndiannessAwareRepresentation.hpp
//
// Created on: Sep 28, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_UTILITY_UTILITY_CONT_ENDIANNESSAWAREREPRESENTATION_HPP
#define COMPONENTS_UTILITY_UTILITY_CONT_ENDIANNESSAWAREREPRESENTATION_HPP

#include <array>
#include <cstdint>
#include <type_traits>

namespace Ut {
namespace Cont {

/// \brief Helps with converting native representation to the target one when
/// packing bytes for transfer
/// \pre The instance MUST reside in memory that does support unaligned access
template <class NumericType, bool kCurrentIsLittleEndian, bool kTargetIsLittleEndian>
union EndiannessAwareRepresentation {
	NumericType nativeRepresentation;
	std::array<std::uint8_t, sizeof(NumericType)> byteArrayRepresentation;

	typename std::array<std::uint8_t, sizeof(NumericType)>::const_iterator cbeginNative() const
	{
		return byteArrayRepresentation.cbegin();
	}

	typename std::array<std::uint8_t, sizeof(NumericType)>::const_iterator cendNative() const
	{
		return byteArrayRepresentation.cend();
	}

	typename std::array<std::uint8_t, sizeof(NumericType)>::const_iterator cbegin() const
	{
		return byteArrayRepresentation.cbegin();
	}

	typename std::array<std::uint8_t, sizeof(NumericType)>::const_iterator cend() const
	{
		return byteArrayRepresentation.cend();
	}

	typename std::array<std::uint8_t, sizeof(NumericType)>::const_iterator crbegin() const
	{
		return byteArrayRepresentation.crbegin();
	}

	typename std::array<std::uint8_t, sizeof(NumericType)>::const_iterator crend() const
	{
		return byteArrayRepresentation.crend();
	}

	template <bool kFlag = (kCurrentIsLittleEndian == kTargetIsLittleEndian)>
	typename std::enable_if<kFlag, typename std::template array<std::uint8_t, sizeof(NumericType)>::const_iterator>::type
	cbeginTarget() const
	{
		return byteArrayRepresentation.cbegin();
	}

	template <bool kFlag = (kCurrentIsLittleEndian == kTargetIsLittleEndian)>
	typename std::enable_if<kFlag, typename std::template array<std::uint8_t, sizeof(NumericType)>::const_iterator>::type
	cendTarget() const
	{
		return byteArrayRepresentation.cend();
	}

	template <bool kFlag = (kCurrentIsLittleEndian == kTargetIsLittleEndian)>
	typename std::enable_if<!kFlag, typename std::template array<std::uint8_t, sizeof(NumericType)>::const_reverse_iterator>::type
	cbeginTarget() const
	{
		return byteArrayRepresentation.crbegin();
	}

	template <bool kFlag = (kCurrentIsLittleEndian == kTargetIsLittleEndian)>
	typename std::enable_if<!kFlag, typename std::template array<std::uint8_t, sizeof(NumericType)>::const_reverse_iterator>::type
	cendTarget() const
	{
		return byteArrayRepresentation.crend();
	}
};

static_assert(sizeof(EndiannessAwareRepresentation<std::uint16_t, true, true>{0}) == sizeof(std::uint16_t{0}), "");
static_assert(sizeof(EndiannessAwareRepresentation<std::uint32_t, true, true>{0}) == sizeof(std::uint32_t{0}), "");
static_assert(sizeof(EndiannessAwareRepresentation<std::uint64_t, true, true>{0}) == sizeof(std::uint64_t{0}), "");

}  // Cont
}  // Ut

#endif // COMPONENTS_UTILITY_UTILITY_CONT_ENDIANNESSAWAREREPRESENTATION_HPP

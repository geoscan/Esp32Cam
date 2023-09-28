#define OHDEBUG_PORT_ENABLE 1
#define OHDEBUG_TAGS_ENABLE "Trace"
#include <OhDebug.hpp>

#include <utility/al/Crc32.hpp>
#include <utility/cont/EndiannessAwareRepresentation.hpp>
#include <algorithm>
#include <array>
#include <cassert>

template <class T>
constexpr const uint8_t* u8PointerCast(T aArg)
{
	return static_cast<const std::uint8_t *>(static_cast<const void *>(aArg));
}

OHDEBUG_TEST("Utility, CRC32, constexpr identical to runtime CRC32 calculation")
{
	constexpr std::array<std::uint32_t, 3> constexprCrc32 {{
		Ut::Al::Crc32Constexpr::calculateCrc32("Hello"),
		Ut::Al::Crc32Constexpr::calculateCrc32("Goodbye"),
		Ut::Al::Crc32Constexpr::calculateCrc32("HelloAgain"),
	}};
	const std::array<std::uint32_t, 3> runtimeCrc32 = {{
		Ut::Al::Crc32::calculateCrc32(u8PointerCast("Hello"), sizeof("Hello")),
		Ut::Al::Crc32::calculateCrc32(u8PointerCast("Goodbye"), sizeof("Goodbye")),
		Ut::Al::Crc32::calculateCrc32(u8PointerCast("HelloAgain"), sizeof("HelloAgain")),
	}};
	OHDEBUG("Trace", "constexpr CRC32",
		constexprCrc32[0],
		constexprCrc32[1],
		constexprCrc32[2],
		"runtime CRC32",
		runtimeCrc32[0],
		runtimeCrc32[1],
		runtimeCrc32[2]);
	assert(constexprCrc32 == runtimeCrc32);
}

OHDEBUG_TEST("Utility, Endianness-aware representation")
{
	constexpr std::uint64_t forwardU64 = 0x0123456789ABDCEF;
	constexpr std::uint64_t reversedU64 = 0xEFDCAB8967452301;
	constexpr std::uint32_t forwardU32 = 0x89ABDCEF;
	constexpr std::uint32_t reversedU32 = 0xEFDCAB89;
	constexpr std::uint32_t forwardU16 = 0x89AB;
	constexpr std::uint32_t reversedU16 = 0xAB89;
	Ut::Cont::EndiannessAwareRepresentation<std::uint32_t, true, true> matchingU32Representation{forwardU32};
	Ut::Cont::EndiannessAwareRepresentation<std::uint32_t, true, false> invertedU32Representation{forwardU32};
	Ut::Cont::EndiannessAwareRepresentation<std::uint32_t, true, true> referenceForwardU32Representation{forwardU32};
	Ut::Cont::EndiannessAwareRepresentation<std::uint32_t, true, true> referenceReversedU32Representation{reversedU32};
	assert(std::equal(matchingU32Representation.cbeginTarget(), matchingU32Representation.cendTarget(), referenceForwardU32Representation.cbegin()));
	assert(std::equal(invertedU32Representation.cbeginTarget(), invertedU32Representation.cendTarget(), referenceReversedU32Representation.cbegin()));
	Ut::Cont::EndiannessAwareRepresentation<std::uint16_t, true, true> matchingU16Representation{forwardU16};
	Ut::Cont::EndiannessAwareRepresentation<std::uint16_t, true, false> invertedU16Representation{forwardU16};
	Ut::Cont::EndiannessAwareRepresentation<std::uint16_t, true, true> referenceForwardU16Representation{forwardU16};
	Ut::Cont::EndiannessAwareRepresentation<std::uint16_t, true, true> referenceReversedU16Representation{reversedU16};
	assert(std::equal(matchingU16Representation.cbeginTarget(), matchingU16Representation.cendTarget(), referenceForwardU16Representation.cbegin()));
	assert(std::equal(invertedU16Representation.cbeginTarget(), invertedU16Representation.cendTarget(), referenceForwardU16Representation.crbegin()));
	Ut::Cont::EndiannessAwareRepresentation<std::uint64_t, true, true> matchingU64Representation{forwardU64};
	Ut::Cont::EndiannessAwareRepresentation<std::uint64_t, true, false> invertedU64Representation{forwardU64};
	Ut::Cont::EndiannessAwareRepresentation<std::uint64_t, true, true> referenceForwardU64Representation{forwardU64};
	Ut::Cont::EndiannessAwareRepresentation<std::uint64_t, true, true> referenceReversedU64Representation{reversedU64};
	assert(std::equal(matchingU64Representation.cbeginTarget(), matchingU64Representation.cendTarget(), referenceForwardU64Representation.cbegin()));
	assert(std::equal(invertedU64Representation.cbeginTarget(), invertedU64Representation.cendTarget(), referenceReversedU64Representation.cbegin()));
}

int main(void)
{
	OHDEBUG("Trace", "utility_test");
	OHDEBUG_RUN_TESTS();

	return 0;
}

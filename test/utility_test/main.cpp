#define OHDEBUG_PORT_ENABLE 1
#define OHDEBUG_TAGS_ENABLE "Trace"
#include <OhDebug.hpp>

#include <utility/al/Crc32.hpp>
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

int main(void)
{
	OHDEBUG("Trace", "utility_test");
	OHDEBUG_RUN_TESTS();

	return 0;
}

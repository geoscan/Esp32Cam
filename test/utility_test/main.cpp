#define OHDEBUG_PORT_ENABLE 1
#define OHDEBUG_TAGS_ENABLE "Trace"
#include <OhDebug.hpp>

#include <utility/al/Crc32.hpp>
#include <cassert>

template <class T>
constexpr const uint8_t* u8PointerCast(T aArg)
{
	return static_cast<const std::uint8_t *>(static_cast<const void *>(aArg));
}

OHDEBUG_TEST("Utility, CRC32, constexpr identical to runtime CRC32 calculation")
{
	constexpr auto constexprCrc32 = Ut::Al::Crc32Constexpr::calculateCrc32<sizeof("Hello")>("Hello");
	const auto runtimeCrc32 = Ut::Al::Crc32::calculateCrc32(u8PointerCast("Hello"),
		sizeof("Hello"));
	OHDEBUG("Trace", "constexpr CRC32", constexprCrc32, "runtime CRC32", runtimeCrc32);
	assert(constexprCrc32 == runtimeCrc32);
}

int main(void)
{
	OHDEBUG("Trace", "utility_test");
	OHDEBUG_RUN_TESTS();

	return 0;
}

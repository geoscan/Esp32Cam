#include <sdkconfig.h>

#include <algorithm>
#include <string>
#include <iterator>
#include <chrono>

#include "version.hpp"
#include "UartSink.hpp"
#include "VersionParser.hpp"

using namespace std;

static VersionParser parser;
static std::string version;

void versionInit()
{
#ifdef CONFIG_VERSION_ACQUISITION_ENABLE

	uint8_t buffer[CONFIG_VERSION_SERIAL_BUFFER_SIZE];
	UartDevice uartDevice(UART_NUM_0, GPIO_NUM_3, GPIO_NUM_1, CONFIG_VERSION_UART_BAUDRATE, UART_PARITY_DISABLE, UART_STOP_BITS_1);

	const auto timeEnd = std::chrono::system_clock::now() + std::chrono::microseconds(CONFIG_VERSION_STM_ACQ_TIMEOUT_US);

	// Acquire and try to parse
	while (std::chrono::system_clock::now() < timeEnd && !parser.parsed()) {
		size_t nread = uartDevice.read(buffer, sizeof(buffer));
		if (nread) {
			parser.parse(buffer, nread);
		}
	}

	// Apply proper formatting
	if (parser.parsed()) {
		const auto &payload = parser.payload();

		if (parser.payload().size() >= 2) {
			version.append(to_string(payload[0]));
			version.append(".");
			version.append(to_string(payload[1]));
		}

		if (payload.size() >= 6) {
			version.append(".");
			version.append(to_string(*reinterpret_cast<const uint32_t *>(&payload.data()[2])));
		}
	}

#endif  // #ifdef CONFIG_VERSION_ACQUISITION_ENABLE
}

std::string connectedSerialVersionGet()
{
#ifdef CONFIG_VERSION_ACQUISITION_ENABLE
	return version;
#else
	return {};
#endif  // #ifdef CONFIG_VERSION_ACQUISITION_ENABLE
}

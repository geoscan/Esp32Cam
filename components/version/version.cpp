#include <sdkconfig.h>

#include <algorithm>
#include <string>
#include <iterator>

#include "version.hpp"
#include "UartSink.hpp"
#include "VersionParser.hpp"

using namespace std;

static VersionParser parser;

static void cbStm32UartInput(const void *data, size_t size)
{
	if (!parser.parsed()) {
		parser.parse(data, size);
	}
}

void versionInit()
{
	UartSink stmSink(UART_NUM_0, GPIO_NUM_3, GPIO_NUM_1, 2000000, UART_PARITY_DISABLE, UART_STOP_BITS_1);

	stmSink.addCallback(cbStm32UartInput);
	stmSink.run(CONFIG_VERSION_STM_ACQ_TIMEOUT_US);
}

bool versionStmGet(string &str)
{
	bool res = false;

	if (parser.parsed()) {
		const auto &payload = parser.payload();
		res = true;

		if (parser.payload().size() >= 2) {
			str.append(to_string(payload[0]));
			str.append(".");
			str.append(to_string(payload[1]));
		}

		if (payload.size() >= 6) {
			str.append(".");
			str.append(to_string(*reinterpret_cast<const uint32_t *>(&payload.data()[2])));
		}
	}

	return res;
}
#include <sdkconfig.h>

#include <algorithm>
#include <string>

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

	if (parser.parsed() && parser.payload().size() >= 2) {
		res = true;
		str.append(to_string(parser.payload()[0]));
		str.append(".");
		str.append(to_string(parser.payload()[1]));
	}

	return res;
}
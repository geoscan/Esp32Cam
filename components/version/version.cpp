#include "version.hpp"
#include "UartSink.hpp"
#include "VersionParser.hpp"
#include <algorithm>

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
	stmSink.run(500000);
}

bool versionStmGet(string &str)
{
	bool res = false;

	if (parser.parsed()) {
		res = true;
		transform(parser.payload().cbegin(), parser.payload().cend(), back_inserter(str),
			[](uint8_t ch) {return static_cast<char>(ch); });
	}

	return res;
}
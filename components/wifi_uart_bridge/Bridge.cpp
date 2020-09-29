#include <sdkconfig.h>
#include <pthread.h>

#include "Bridge.hpp"

#ifndef CONFIG_WIFI_UART_BRIDGE_RX_TX_BUFFER
#define CONFIG_WIFI_UART_BRIDGE_RX_TX_BUFFER 512
#endif  // CONFIG_WIFI_UART_BRIDGE_RX_TX_BUFFER

Bridge::Bridge(Endpoint &e1, Endpoint &e2) : first(e1), second(e2)
{
}

void Bridge::run()
{
	EpPair ep1 {&first, &second};
	EpPair ep2 {&second, &first};
	pthread_t stub;

	pthread_create(&stub, 0, bridgeTask, &ep2);
	bridgeTask(&ep1);
}

void *Bridge::bridgeTask(void *endpoints)
{
	EpPair &ep = *reinterpret_cast<EpPair *>(endpoints);
	char buf[kBufSize];

	while (true) {
		performTransfer(*ep.first, *ep.second, asio::mutable_buffer(buf, kBufSize));
	}
}

void Bridge::performTransfer(Endpoint &a, Endpoint &b, asio::mutable_buffer buf)
{
	size_t nrecv = a.read(buf);
	if (nrecv > 0) {
		b.write({buf.data(), nrecv});
	}
}
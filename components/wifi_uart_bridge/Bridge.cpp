#include <sdkconfig.h>
#include <pthread.h>

#include "Bridge.hpp"

#ifndef CONFIG_WIFI_UART_BRIDGE_RX_TX_BUFFER
#define CONFIG_WIFI_UART_BRIDGE_RX_TX_BUFFER 512
#endif  // CONFIG_WIFI_UART_BRIDGE_RX_TX_BUFFER

Bridge::Bridge(Endpoint &e1, Endpoint &e2) : first(epa), second(epb)
{
}

void Bridge::run()
{
	Endpoints routeAb{first, second};
	Endpoints routeBa{second, first};
	pthread_t pth;

	pthread_create(&pth, NULL, bridgingRoutine, &routeAb);
	bridgingRoutine(&routeBa);
}

void *Bridge::bridgingRoutine(void *arg)
{
	Endpoints endpoints = *(reinterpret_cast<Endpoints *>(arg));
	size_t    nrecv      = 0;
	char      iobuf[CONFIG_WIFI_UART_BRIDGE_RX_TX_BUFFER];

	while (true) {
		nrecv = endpoints.first.read(asio::mutable_buffer(iobuf, CONFIG_WIFI_UART_BRIDGE_RX_TX_BUFFER));
		if (nrecv > 0) {
			endpoints.second.write(asio::const_buffer(iobuf, nrecv));
		}
	}
}
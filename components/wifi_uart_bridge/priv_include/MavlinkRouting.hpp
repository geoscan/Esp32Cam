//
// MavlinkRouting.hpp
//
// Created on: Aug 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(WIFI_UART_BRIDGE_PRIV_INCLUDE_WIFI_UART_BRIDGE_MAVLINKROUTING_HPP_)
#define WIFI_UART_BRIDGE_PRIV_INCLUDE_WIFI_UART_BRIDGE_MAVLINKROUTING_HPP_

#include "sub/Rout.hpp"
#include <vector>

namespace Bdg {

class LambdaReceiver;

/// \brief Encapsulates MAVLink-related routing. Provides RAII-based access to MAVLink routing capabilities.
///
class MavlinkRouting final {
public:
	MavlinkRouting();
	~MavlinkRouting();

	static constexpr std::uint16_t getMavlinkUdpPort()
	{
		return 8001;
	}

	static constexpr std::uint8_t getMavlinkUartNum()
	{
		return 1;
	}

private:
	std::vector<asio::ip::udp::endpoint> clientsUdp;
	std::vector<LambdaReceiver> receivers;  ///< Those mock receiving endpoints (UARTs and IP sockets), so there is no need to complicate implementation of the respective
};

}  // namespace Bdg

#endif // WIFI_UART_BRIDGE_PRIV_INCLUDE_WIFI_UART_BRIDGE_MAVLINKROUTING_HPP_

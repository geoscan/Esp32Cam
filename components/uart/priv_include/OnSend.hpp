//
// OnSend.hpp
//
// Created on: Jan 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Provides send "service" through a callback mechanism implemented by Rr
// library. See Utility/Subscriptions.hpp for more details.

#ifndef UART_PRIV_INCLUDE_ONSEND_HPP
#define UART_PRIV_INCLUDE_ONSEND_HPP

#include "sub/Subscription.hpp"
#include <array>

class UartDevice;

namespace Uart {

template <unsigned N>
using OnSendBase = std::array<UartDevice *, N>;

///
/// \brief Handles UART send requests through use of Rr lib-based subscription
/// mechanism
///
/// \tparam N - number of uart interfaces
///
template <unsigned N>
class OnSend : OnSendBase<N> {
	struct {
		Sub::Key::MavlinkUartSend mavlinkUartSend;
	} key;

private:
	Sub::UartSendResult send(const Sub::UartMessage &);

public:
	template <class ...Ta>
	OnSend(Ta &&...);

	Sub::UartSendResult onMavlinkSend(Sub::Message &aMessage);
};

}  // namespace Uart

#include "OnSend.impl"

#endif  // UART_PRIV_INCLUDE_ONSEND_HPP

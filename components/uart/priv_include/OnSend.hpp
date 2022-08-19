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
#include "sub/Rout.hpp"
#include <array>

namespace Utility {
namespace Sys {
class UartDevice;
}  // namespace Sys
}  // namespace Utility

namespace Uart {

template <unsigned N>
using OnSendBase = std::array<Utility::Sys::UartDevice *, N>;

///
/// \brief Handles UART send requests through use of Rr lib-based subscription
/// mechanism
///
/// \tparam N - number of uart interfaces (overall). Storing pointers in an array enables faster access.
///
template <unsigned NuartInterfaces>
class OnSend : OnSendBase<NuartInterfaces> {
	struct {
		Sub::Rout::UartSend uartSend;
	} key;

private:
	Sub::Rout::UartSend::Ret send(Sub::Rout::UartSend::Arg<0>);

public:
	template <class ...Ta>
	OnSend(Ta &&...);
};

}  // namespace Uart

#include "OnSend.impl"

#endif  // UART_PRIV_INCLUDE_ONSEND_HPP

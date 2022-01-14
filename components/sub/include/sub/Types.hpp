//
// Types.hpp
//
// Created on: Jan 12, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SUB_INCLUDE_TYPES_HPP
#define SUB_INCLUDE_TYPES_HPP

#include "utility/Buffer.hpp"

namespace Sub {

// Various typedefs pertaining to subscription mechanisms

using Port = std::uint16_t;  ///< TCP / UDP port
using Ip4 = std::uint8_t[4];  ///< IPv4 Address

// Atomic entities serving as shortcuts for concrete types used in routing and
// different control commands.

struct IpTransport {
	enum {
		Tcp,
		Udp,
	} transport;  ///< A type of IP transport that is being used
};

struct IpEndpoint {
	Ip4 address;  ///< Remote IP address
	Port port;  ///< Remote IP port
};

struct IpEndpointHost {
	Port hostPort;  ///< Local IP port
};

enum class UartNum : int {
	Mavlink = 0,
};

struct UartEndpoint {
	UartNum uartNum;  ///< Id. of UART interface being used
};

struct Message {
	Utility::ConstBuffer payload;  ///< Raw bytes transferred with an interface
};

// Derivatives of the atomic entities defined above

///
/// \brief In/Out message. Message buffer + info on UART used
///
struct UartMessage : Message, UartEndpoint {
};

///
/// \brief Incoming message
///
struct IpMessage : Message, IpEndpoint, IpTransport {
};

///
/// \brief Outgoing message
///
struct IpDestMessage : IpMessage, IpEndpointHost {
};

///
/// \brief Socket control operations
///
struct IpDestEndpoint : IpEndpoint, IpEndpointHost {
};

///
/// \brief Encapsulation of a command evaluated upon an IP interface
///
struct IpConnect : IpDestEndpoint {
	bool connect;  ///< true for connect, false for disconnect
};

// Routing and control result types

///
/// \brief A generic return. It can be detailed through using an additional
/// subject-specific enum
///
enum class ResultCode {
	None = -1,  ///< A request cannot be fulfilled by this particular provider
	Success = 0,
	Fail,

	Max,
};

///
/// \brief A generic structure storing result of an operation
///
struct ResultGeneric {
	ResultCode resultCode;  ///< Generic result code
};

///
/// \brief Encapsulates all the resulting information acquired from making an
/// IP request such as connect or disconnect.
///
struct IpResult : ResultGeneric {
};

///
/// \brief Result of processing an incoming packet
///
struct MavReceiveResult {
};

///
/// \brief Dedicated structure to store a result of sending a byte package
///
struct SendResult : ResultGeneric {
};

///
/// \brief Dedicated structure to store a result of sending a byte package over IP
///
struct IpSendResult : SendResult {
};

///
/// \brief Dedicated structure to store a result of sending a byte package over UART
///
struct UartSendResult : SendResult {
};

}  // namespace Sub

#endif  // SUB_INCLUDE_TYPES_HPP

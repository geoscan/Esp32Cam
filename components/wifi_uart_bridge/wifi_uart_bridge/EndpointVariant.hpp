//
// EndpointVariant.hpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <mapbox/variant.hpp>
#include <asio.hpp>
#include <tuple>

#if !defined(WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ENDPOINTVARIANT_HPP_)
#define WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ENDPOINTVARIANT_HPP_

namespace Bdg {

/// \brief Some endpoints do not need additional identification (such as uart num or port).
///
enum class NamedEndpoint {
	Mavlink,  ///< Mavlink protocol implementation
	MavlinkIpPack,  ///< Pack and forward data received over IP
	UartMavlinkForwarded,  ///< Mavlink packets forwarded from a dedicated UART
	UdpMavlinkForwarded,  ///< Mavlink packets forwarded from a dedicated UDP port
};

using TcpEndpoint = typename std::tuple<asio::ip::tcp::endpoint, std::uint16_t /* Port */>;
using UdpEndpoint = typename std::tuple<asio::ip::udp::endpoint, std::uint16_t /* Port */>;
using UartEndpoint = typename std::tuple<std::uint8_t /* UART num */>;
struct TcpPort : std::tuple<std::uint16_t> {  // Inheritance from `std::tuple` is used to differentiate b/w types while retaining comparison operators implemented for `std::tuple`.
	using std::tuple<std::uint16_t>::tuple;
};
struct UdpPort : std::tuple<std::uint16_t> {
	using std::tuple<std::uint16_t>::tuple;
};

using EndpointVariant = mapbox::util::variant<NamedEndpoint, TcpEndpoint, UdpEndpoint, UartEndpoint>;

/// \brief Enalbes using rules that apply on mere equality of originators' ports, ignoring addresses of the actual
/// remote senders
///
bool operator<(const TcpEndpoint &aLhs, const TcpPort &aRhs);
bool operator<(const UdpEndpoint &aLhs, const UdpPort &aRhs);
bool operator==(const TcpEndpoint &aLhs, const TcpPort &aRhs);
bool operator==(const UdpEndpoint &aLhs, const UdpPort &aRhs);

}  // namespace Bdg

#endif // WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ENDPOINTVARIANT_HPP_

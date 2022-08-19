//
// EndpointVariant.hpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <mapbox/variant.hpp>
#include <asio.hpp>
#include <tuple>
#include "utility/cont/PosArray.hpp"

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
	MavlinkIpPackForwarded,  ///< IP packets, wrapped and forwarded to UART
	None,  ///< Stub meaning that no further reduction is required
};

/// \brief Makes it possible to distinguish between endpoint types.
///
/// \details Event w/ use of inheritance from `std::tuple`, inherited types will produce ambiguity in certain contexts,
/// but it is not the case, when Marker is used.
///
template <class T>
struct EndpointMarker {
};

template <class T>
constexpr bool operator<(const EndpointMarker<T> &aLhs, const EndpointMarker<T> &aRhs)
{
	return false;
}

template <class T>
constexpr bool operator==(const EndpointMarker<T> &aLhs, const EndpointMarker<T> &aRhs)
{
	return true;
}

template <class T>
constexpr bool operator!=(const EndpointMarker<T> &aLhs, const EndpointMarker<T> &aRhs)
{
	return false;
}

using TcpEndpoint = typename std::tuple<asio::ip::tcp::endpoint, std::uint16_t /* Port */>;
using UdpEndpoint = typename std::tuple<asio::ip::udp::endpoint, std::uint16_t /* Port */>;
struct MarkerUart;
using UartEndpoint = typename std::tuple<std::uint8_t /* UART num */, EndpointMarker<MarkerUart>>;
struct MarkerTcpPort;
using TcpPort = typename std::tuple<std::uint16_t, EndpointMarker<MarkerTcpPort>>;
struct MarkerUdpHook;
using UdpHook = typename std::tuple<std::uint16_t, EndpointMarker<MarkerUdpHook>>;

struct UdpPort : std::tuple<std::uint16_t> {
	using std::tuple<std::uint16_t>::tuple;
};

using EndpointVariantBase = mapbox::util::variant<NamedEndpoint, TcpEndpoint, UdpEndpoint, UartEndpoint,
	asio::ip::udp::endpoint, asio::ip::tcp::endpoint, TcpPort, UdpPort, UdpHook>;

struct EndpointVariant : EndpointVariantBase
{
	static constexpr auto kPosArraySize = 2;
	using EndpointVariantBase::EndpointVariantBase;
	Ut::Cont::PosArray<EndpointVariant, kPosArraySize> asAlternative() const;
	void logi(const char *prefix) const;
	void logv(const char *prefix) const;
};

}  // namespace Bdg

#endif // WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ENDPOINTVARIANT_HPP_

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
	None,
};

namespace EndpointVariantImpl {

struct TcpPort;
struct UdpPort;

/// \brief Stub<T> enables differentiation b/w tuples of similar content which is important during comparison
///
template <class T>
struct Stub {};

template <class T>
constexpr bool operator==(const Stub<T> &, const Stub<T> &)
{
	return true;
}

}  // namespace EndpointVariantImpl

using TcpEndpoint = typename std::tuple<asio::ip::tcp::endpoint, std::uint16_t /* Port */>;
using UdpEndpoint = typename std::tuple<asio::ip::udp::endpoint, std::uint16_t /* Port */>;
using UartEndpoint = typename std::tuple<std::uint8_t /* UART num */>;
using TcpPort = typename std::tuple<std::uint16_t /* port */,
	typename EndpointVariantImpl::Stub<EndpointVariantImpl::TcpPort>>;
using UdpPort = typename std::tuple<std::uint16_t /* port */,
	typename EndpointVariantImpl::Stub<EndpointVariantImpl::UdpPort>>;

using EndpointVariant = mapbox::util::variant<NamedEndpoint, TcpEndpoint, UdpEndpoint, UartEndpoint>;

/// \brief Enalbes using rules that apply on mere equality of originators' ports, ignoring addresses of the actual
/// remote senders
///
bool operator==(const TcpEndpoint &aLhs, const TcpPort &aRhs);
bool operator==(const UdpEndpoint &aLhs, const UdpPort &aRhs);

}  // namespace Bdg

#endif // WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ENDPOINTVARIANT_HPP_

//
// EndpointVariant.cpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)

#include "EndpointVariant.hpp"
#include "utility/LogSection.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
#include <esp_log.h>

GS_UTILITY_LOGV_METHOD_SET_ENABLED(Bdg::EndpointVariant, logv, 1)

namespace Bdg {

static constexpr const char *kNamedEndpoints[] = {
	"Mavlink",
	"MavlinkIpPack",
	"UartMavlinkForwarded",
	"UdpMavlinkForwarded",
	"MavlinkIpPackForwarded",
	"None"
};

static_assert(sizeof(kNamedEndpoints) / sizeof(kNamedEndpoints[0]) == (std::size_t)NamedEndpoint::None + 1,
	"NamedEndpoint to NAME mapping should be a 1:1 match");

static const char *namedEndpointAsStr(NamedEndpoint aEndpoint)
{
	return kNamedEndpoints[static_cast<std::size_t>(aEndpoint)];
}

/// \brief Enables additional interpretation of an identity
///
/// \details In some cases, we only need to apply rules based on targets' ports or remote senders' identities. E.g.
/// (Sender endpoint, Port) pairs may also be interpreted as (Sender endpoint) or (Port). `asAlternative` performs
/// match against entities that can be decomposed or re-interpreted, and fills the return vector w/ the results of that
/// decomposition.
///
Utility::PosArray<EndpointVariant, EndpointVariant::kPosArraySize> EndpointVariant::asAlternative() const
{
	Utility::PosArray<EndpointVariant, kPosArraySize> ret{};
	match(
		[&ret](const UdpEndpoint &aUdpEndpoint)
		{
			static_assert(kPosArraySize >= 2, "Insufficient array size");
			ret.emplace_back(std::get<0>(aUdpEndpoint));
			ret.emplace_back(UdpPort(std::get<1>(aUdpEndpoint)));
		},
		[&ret](const TcpEndpoint &aTcpEndpoint)
		{
			static_assert(kPosArraySize >= 2, "Insufficient array size");
			ret.emplace_back(std::get<0>(aTcpEndpoint));
			ret.emplace_back(TcpPort{std::get<1>(aTcpEndpoint), {}});
		},
		[](...) {});

	return ret;
}

#if CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL >= 5

/// \brief If the logging level is high enough, prints its own content into logv
///
void EndpointVariant::logv(const char *aPrefix) const
{
	match(
		[aPrefix](NamedEndpoint a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv,
			"%s NamedEndpoint::%s", aPrefix, namedEndpointAsStr(a)); },
		[aPrefix](const TcpEndpoint &a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv, "%s TcpEndpoint"
			"remote %s:%d local port %d", aPrefix, std::get<0>(a).address().to_string().c_str(), std::get<0>(a).port(),
			std::get<1>(a)); },
		[aPrefix](const UartEndpoint &a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv, "%s"
			"UartEndpoint %d", aPrefix, std::get<0>(a)); },
		[aPrefix](const UdpEndpoint &a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv, "%s UdpEndpoint"
			"remote %s:%d local port %d", aPrefix, std::get<0>(a).address().to_string().c_str(), std::get<0>(a).port(),
			std::get<1>(a)); },
		[aPrefix](const asio::ip::tcp::endpoint &a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv,
			"%s asio::ip::tcp::endpoint %s port %d", aPrefix, a.address().to_string().c_str(), a.port()); },
		[aPrefix](const asio::ip::udp::endpoint &a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv,
			"%s asio::ip::udp::endpoint %s port %d", aPrefix, a.address().to_string().c_str(), a.port()); },
		[aPrefix](const UdpPort &a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv, "%s UdpPort %d",
			aPrefix, std::get<0>(a)); },
		[aPrefix](const UdpHook &a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv, "%s UdpHook %d",
			aPrefix, std::get<0>(a)); },
		[aPrefix](const TcpPort &a) {GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, EndpointVariant, logv, "%s TcpPort %d",
			aPrefix, std::get<0>(a)); },
		[aPrefix](...){GS_UTILITY_LOGV_METHOD(kDebugTag, EndpointVariant, logv, "%s UNHANDLED VARIANT CONTENT",
			aPrefix); }
	);
}

#else

void EndpointVariant::logv()
{
}

#endif

}  // namespace Bdg

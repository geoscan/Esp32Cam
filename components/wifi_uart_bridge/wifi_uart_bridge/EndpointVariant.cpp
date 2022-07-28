//
// EndpointVariant.cpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "EndpointVariant.hpp"

namespace Bdg {

/// \brief Enables additional interpretation of an identity
///
/// \details In some cases, we only need to compare rules by target ports or remote senders' identities. E.g. (Sender
/// endpoint, Port) pairs may also be interpreted as (Sender endpoint) or (Port). `asAlternative` generalizes this
/// feature.
///
std::vector<EndpointVariant> EndpointVariant::asAlternative()
{
	constexpr std::size_t kSizeHint = 2;
	std::vector<EndpointVariant> ret{kSizeHint};
	match(
		[&ret](const UdpEndpoint &aUdpEndpoint)
		{
			ret.emplace_back(std::get<0>(aUdpEndpoint));
			ret.emplace_back(UdpPort{std::get<1>(aUdpEndpoint)});
		},
		[&ret](const TcpEndpoint &aTcpEndpoint)
		{
			ret.emplace_back(std::get<0>(aTcpEndpoint));
			ret.emplace_back(TcpPort{std::get<1>(aTcpEndpoint)});
		},
		[](...) {});

	return ret;
}

}  // namespace Bdg

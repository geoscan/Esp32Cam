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
/// \details In some cases, we only need to apply rules based on targets' ports or remote senders' identities. E.g.
/// (Sender endpoint, Port) pairs may also be interpreted as (Sender endpoint) or (Port). `asAlternative` performs
/// match against entities that can be decomposed or re-interpreted, and fills the return vector w/ the results of that
/// decomposition.
///
Utility::PosArray<EndpointVariant, EndpointVariant::kPosArraySize> EndpointVariant::asAlternative()
{
	Utility::PosArray<EndpointVariant, kPosArraySize> ret{};
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

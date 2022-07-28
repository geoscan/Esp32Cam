//
// EndpointVariant.cpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "EndpointVariant.hpp"

namespace Bdg {

bool operator==(const TcpEndpoint &aLhs, const TcpPort &aRhs)
{
	return std::get<1>(aLhs) == std::get<0>(aRhs);
}

bool operator==(const UdpEndpoint &aLhs, const UdpPort &aRhs)
{
	return std::get<1>(aLhs) == std::get<0>(aRhs);
}

bool operator<(const TcpEndpoint &aLhs, const TcpPort &aRhs)
{
	return std::get<1>(aLhs) < std::get<0>(aRhs);
}

bool operator<(const UdpEndpoint &aLhs, const UdpPort &aRhs)
{
	return std::get<1>(aLhs) < std::get<0>(aRhs);
}

}  // namespace Bdg

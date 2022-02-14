//
// Rout.cpp
//
// Created on: Feb 14, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "sub/Rout.hpp"

namespace Sub {
namespace Rout {

Response::Type Response::getType()
{
	if (payload.data() != nullptr) {
		return Type::Response;
	} else {
		return static_cast<Type>(payload.size());
	}
}

void Response::setType(Type aType)
{
	if (aType != Type::Response) {
		payload = Payload{nullptr, static_cast<std::size_t>(aType)};
	}
}

}  // namespace Rout
}  // namespace Sub

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

Response::Response(Response::Type aType) : payload{}, payloadHold{}, nProcessed{-1}
{
	setType(aType);
}

Response::Response() : Response(Type::Ignored)
{

}

}  // namespace Rout
}  // namespace Sub

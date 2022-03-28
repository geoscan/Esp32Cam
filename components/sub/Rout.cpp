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

///
/// Release the lock or the buffer, if either was acquired
///
/// \note If a `Response` object is reused, `reset()` shold be invoked on every iteration. Otherwise, it creates a
/// possiblity for deadlocks to appear.
///
void Response::reset()
{
	payloadHold.reset();
	payloadLock.reset();
}

}  // namespace Rout
}  // namespace Sub

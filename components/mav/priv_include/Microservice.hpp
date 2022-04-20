//
// Microservice.hpp
//
// Created on: Dec 21, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// We understand an instance of MAVLink microservice as an inherently cohesive
// structure which encapsulates related pieces of functionality. It overlaps
// with the notion of microservice as it is defined by the protocol developers;
// these two are often used interchangeably.

#ifndef MAV_PRIV_INCLUDEMICROSERVICE_HPP
#define MAV_PRIV_INCLUDEMICROSERVICE_HPP

#include <functional>

struct __mavlink_message;
using mavlink_message_t = __mavlink_message;

namespace Mav {

struct Microservice {
	///
	/// \brief Return type for microservice
	///
	enum class Ret {
		Ignored,  ///< This Microservice instance does not handle such messages
		Response,  ///< The message passed as a parameter now contains response MAVLink package
		NoResponse,  ///< The message has been processed, no response required
	};

	using OnResponseSignature = std::function<void(mavlink_message_t &)>;

	///
	/// \brief  Processes a message it is given
	///
	/// \arg aMessage is left non-const intentially, so no unnecessary copyings
	/// will be required. However, it is required that no Microservice instance
	/// would change it unless it returns true
	///
	/// \return True, if the message has been accepted. False otherwise.
	///
	virtual Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) = 0;

	virtual ~Microservice() = default;
};

}  // namespace Mav

#endif  // MAV_PRIV_INCLUDEMICROSERVICE_HPP

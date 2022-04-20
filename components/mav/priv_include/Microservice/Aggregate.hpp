//
// Aggregate.hpp
//
// Created on: Dec 23, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_MICROSERVICE_AGGREGATE_HPP
#define MAV_MICROSERVICE_AGGREGATE_HPP

#include "Microservice.hpp"
#include <array>
#include <memory>

namespace Mav {
namespace Mic {

///
/// @brief  Aggregate microsrevice that polls every nested microservice whether
/// it can fulfill the request represented by mavlink_message_t (see ::process).
///
/// @tparam Tmics Microservice types inherited from Mav::Microservice
///
template <class ...Tmics>
struct Aggregate : Microservice {
	Aggregate();
	Ret process(mavlink_message_t &aMessage, OnResponseSignature) override;

private:
	std::array<std::unique_ptr<Microservice>, sizeof...(Tmics)> microservices;
};

#include "Aggregate.impl"

}  // namespace Mic
}  // namespace Mav

#endif  // MAV_MICROSERVICE_AGGREGATE_HPP

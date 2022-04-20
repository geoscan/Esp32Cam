//
// Camera.hpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_MICROSERVICE_CAMERA_HPP_
#define MAV_PRIV_INCLUDE_MICROSERVICE_CAMERA_HPP_

#include "Microservice.hpp"
#include "DelayedSend.hpp"
#include "utility/HrTimer.hpp"

namespace Mav {
namespace Mic {

class Camera : public Microservice, public Utility::HrTimer, public Mav::DelayedSend {
public:  // Microservice API
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override final;

public:  // Utility::HrTimer API
	void onHrTimer() override final;
};

}  // namespace Mic
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_CAMERA_HPP_

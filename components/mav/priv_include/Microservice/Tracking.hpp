//
// Tracking.hpp
//
// Created on: Nov 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MAV_PRIV_INCLUDE_MICROSERVICE_TRACKING_HPP_)
#define MAV_PRIV_INCLUDE_MICROSERVICE_TRACKING_HPP_

#include "Microservice.hpp"
#include "DelayedSend.hpp"
#include "utility/system/HrTimer.hpp"
#include "utility/cont/CircularBuffer.hpp"
#include "sub/Tracking.hpp"

namespace Mav {
namespace Mic {

class Tracking final : public Microservice, public Ut::Sys::HrTimer, public Mav::DelayedSend {
private:
	/// \brief Event keys
	struct Key {
		Sub::Trk::OnMosseTrackerUpdate onMosseTrackerUpdate;
	};
public:
	Tracking();
	~Tracking();
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override;
	Ret processRequestMessage(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	void onHrTimer() override;
private:
	Key key;
};

}  // namespace Mic
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_TRACKING_HPP_

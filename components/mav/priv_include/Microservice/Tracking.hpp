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

/// \brief Provides MAVLink API to the tracking functionality.
///
/// \note By 2022-11-02, MAVLink tracking functionality is still WIP. However, I've been told that tracking-related
/// messages are here to stay.
/// Link 1: https://github.com/mavlink/mavlink/issues/1669
/// Link 2: https://groups.google.com/g/mavlink/c/riVSIkXLek0
/// This API is not 100% stable, but no major changes are expected in it.
/// Dmtiry Murashov
///
/// \details The following messages are implemented:
/// - MAV_CMD_CAMERA_TRACK_RECTANGLE (https://mavlink.io/en/messages/common.html#MAV_CMD_CAMERA_TRACK_RECTANGLE)
/// 	-  Used as per the protocol
/// - MAV_CMD_CAMERA_STOP_TRACKING (https://mavlink.io/en/messages/common.html#MAV_CMD_CAMERA_TRACK_RECTANGLE)
/// 	- Used as per the protocol
/// - CAMERA_TRACKING_IMAGE_STATUS (https://mavlink.io/en/messages/common.html#CAMERA_TRACKING_IMAGE_STATUS)
/// 	- Used as per the protocol
class Tracking final : public Microservice, public Mav::DelayedSend {
private:
	/// \brief Event keys
	struct Key {
		Sub::Trk::OnMosseTrackerUpdate onMosseTrackerUpdate;
	};
public:
	Tracking();
	~Tracking();
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override;
	Ret processSetMessageInterval(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	void onMosseTrackerUpdate(Sub::Trk::MosseTrackerUpdate);
private:
	static mavlink_debug_vect_t debugVectMakeFrom(const Sub::Trk::MosseTrackerUpdate &aOnosseTrackerUpdate);
private:
	Key key;
};

}  // namespace Mic
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_TRACKING_HPP_

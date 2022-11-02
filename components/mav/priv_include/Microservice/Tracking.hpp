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
#include "sub/Module.hpp"

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
/// - MAV_CMD_SET_MESSAGE_INTERVAL (https://mavlink.io/en/messages/common.html#MAV_CMD_SET_MESSAGE_INTERVAL)
/// 	- `param2`: only `-1` and `0` values are supported
/// 		- `-1` - Do not track
/// 		- `0` - Send message on every frame
/// - MAV_CMD_ACK (https://mavlink.io/en/messages/common.html#MAV_CMD_ACK)
/// 	- Used as per the protocol
///
/// Sequence to start tracking:
/// - Client sends MAV_CMD_CAMERA_TRACK_RECTANGLE
/// - Tracker responds MAV_CMD_ACK
/// - Tracker starts sending CAMERA_TRACKING_IMAGE_STATUS
///
/// Sequence to stop tracking:
/// - Cilent sends MAV_CMD_CAMERA_STOP_TRACKING
/// - Tracker responds MAV_CMD_ACK
/// - Tracker stops sending CAMERA_TRACKING_IMAGE_STATUS, if it has not already been disabled
///
/// Sequence to control the flow of CAMERA_TRACKING_IMAGE_STATUS messages
/// - Client sends MAV_CMD_SET_MESSAGE_INTERVAL
/// - Tracker adjusts frequency appropriately
class Tracking final : public Microservice, public Mav::DelayedSend {
private:
	/// \brief Event keys
	struct Key {
		Sub::Trk::OnMosseTrackerUpdate onMosseTrackerUpdate;
	};

	/// \brief Relevant cached parts of the camera's state (configuration)
	struct CameraState {
		int frameWidth = 0;
		int frameHeight = 0;
		inline bool isInitialized() const
		{
			return frameWidth != 0 && frameHeight != 0;
		}
		/// \brief Flushes the cached state
		inline void reset()
		{
			frameWidth = 0;
			frameHeight = 0;
		}
		/// \brief Renews the cached state
		void fetch();
	};
public:
	Tracking();
	~Tracking();
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override;
	Ret processCmdSetMessageInterval(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	Ret processCmdCameraTrackRectangle(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	Ret processCmdCameraStopTracking(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	void onMosseTrackerUpdate(Sub::Trk::MosseTrackerUpdate);
private:
	Key key;
	CameraState cameraState;
};

}  // namespace Mic
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_TRACKING_HPP_

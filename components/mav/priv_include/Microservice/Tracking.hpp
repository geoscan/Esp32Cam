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
/// \details MAVLink protocol provides relevant messages for interfacing w/ a tracking implementor. However, as per
/// 2022-11-01, its API is unstable (WIP), and its use is discrouraged.
///
/// The following messages are used for tracking control:
///
/// MAV_CMD_SET_MESSAGE_INTERVAL (https://mavlink.io/en/messages/common.html#MAV_CMD_SET_MESSAGE_INTERVAL)
/// Parameters:
/// `param 1` - Message ID - id of the message. Should be 250 (DEBUG_VECT)
/// `param 2` - Interval.
///    - `-1` for disabling the tracker
///    - `0` for sending the message on a per-frame basis
///
/// DEBUG_VECT(https://mavlink.io/en/messages/common.html#DEBUG_VECT)
/// `time_usec` - as per protocol
/// `x` - packed field containing 2 x u16 values
///   - bits [0; 15] - ROI center, x
///   - bits [16; 32] - frame size, x
/// 'y' - packed field containing 2 x u16 values
///   - bits [0; 15] - ROI center, y
///   - bits [16, 31] - frame size, y
/// 'z' - PSR value (Peak-to-Sidelobe ratio, google MOSSE tracker)
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

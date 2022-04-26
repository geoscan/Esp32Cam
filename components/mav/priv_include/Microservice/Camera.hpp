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
#include "utility/CircularBuffer.hpp"

namespace Mav {
namespace Mic {

class Camera final : public Microservice, public Utility::Tim::HrTimer, public Mav::DelayedSend {
public:
	Camera();

public:  // Microservice API
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override final;

public:  // Utility::HrTimer API
	Ret processRequestMessageCameraInformation(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	Ret processRequestMessageCameraImageCaptured(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	Ret processRequestMessageCameraCaptureStatus(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	Ret processCmdImageStartCapture(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage,
		OnResponseSignature aOnResponse);
	void onHrTimer() override final;

private:

	using ImageName = std::uint16_t;

	struct ImageCapture {
		int sequence;
		bool result;
		ImageName imageName;
		int imageIndex;
	};

	struct History {
		Utility::CircularBuffer<ImageCapture, 4, true> imageCaptureSequence;  ///< Sequence numbers of processed capture requests
		std::int32_t imageCaptureCount = 0;
	} history;
};

}  // namespace Mic
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_CAMERA_HPP_

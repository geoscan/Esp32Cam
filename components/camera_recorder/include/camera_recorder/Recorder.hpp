//
// StreamMediator.hpp
//
// Created on: Mar 24, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// Intermediary class that implements frame processing.
//

#ifndef CAMERA_RECORDER_INCLUDE_CAMERA_RECORDER_STREAMMEDIATOR_HPP
#define CAMERA_RECORDER_INCLUDE_CAMERA_RECORDER_STREAMMEDIATOR_HPP

#include "utility/Subscription.hpp"

namespace CameraRecorder {

class Recorder {
private:
	Utility::Subscription::Key::NewFrame key;
public:

	virtual void newFrame(const std::shared_ptr<Ov2640::Image> &)
	{
	}

	virtual void recordStart(const std::string &)
	{
	}

	virtual void recordStop()
	{
	}

	Recorder() : key{&Recorder::newFrame, this}
	{
	}

	virtual ~Recorder() = default;
};

}  // namespace CameraRecorder

#endif  // CAMERA_RECORDER_INCLUDE_CAMERA_RECORDER_STREAMMEDIATOR_HPP

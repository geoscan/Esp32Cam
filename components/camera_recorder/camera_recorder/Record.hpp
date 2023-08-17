//
// Record.hpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_RECORDER_HPP
#define CAMERA_RECORDER_CAMERA_RECORDER_RECORDER_HPP

#include <vector>
#include "sub/Subscription.hpp"

namespace CameraRecorder {

class Record {
protected:
	using Key = Sub::Key::NewFrame;  // Smart pointer to smth. having "data()" and "size()" methods, with automatic deallocation
	Key key;
	virtual void onNewFrame(Sub::Key::NewFrameEvent) = 0;
public:
	Record();
	virtual bool start(const char *filename) = 0;
	virtual void stop() = 0;
	virtual ~Record();
};

}  // namespace CameraRecorder

#endif // CAMERA_RECORDER_CAMERA_RECORDER_RECORDER_HPP

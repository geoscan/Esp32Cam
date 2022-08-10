//
// Storage.hpp
//
// Created on: May 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(CAMERA_RECORDER_CAMERA_RECORDER_STORAGE_HPP_)
#define CAMERA_RECORDER_CAMERA_RECORDER_STORAGE_HPP_

#include "module/ModuleBase.hpp"
#include <esp_err.h>

namespace CameraRecorder {

/// \brief Encapsulates various aspects of working w/ a file system in the context of storing photo/video frames
///
struct Storage : Mod::ModuleBase {
	Storage();
	void getFieldValue(Mod::Fld::Req, Mod::Fld::OnResponseCallback) override;
	static esp_err_t countFrames(unsigned &aCountOut);
};

}  // namespace CameraRecorder

#endif // CAMERA_RECORDER_CAMERA_RECORDER_STORAGE_HPP_

//
// Storage.hpp
//
// Created on: May 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(CAMERA_RECORDER_CAMERA_RECORDER_STORAGE_HPP_)
#define CAMERA_RECORDER_CAMERA_RECORDER_STORAGE_HPP_

#include "sub/Sys.hpp"
#include <esp_err.h>

namespace CameraRecorder {

/// \brief Encapsulates various aspects of working w/ a file system in the context of storing photo/video frames
///
struct Storage : Sub::Sys::ModuleBase {
	Storage();
	virtual typename Sub::Sys::Fld::ModuleGetFieldMult::Ret getFieldValue(
		typename Sub::Sys::Fld::ModuleGetFieldMult::Arg<0>,
		typename Sub::Sys::Fld::ModuleGetFieldMult::Arg<1>) override;
	static esp_err_t countFrames(unsigned &aCountOut);
	static Sub::Sys::Fld::ModuleGetField::Ret moduleGetField(typename Sub::Sys::Fld::ModuleGetField::Arg<0> arg);
};

}  // namespace CameraRecorder

#endif // CAMERA_RECORDER_CAMERA_RECORDER_STORAGE_HPP_

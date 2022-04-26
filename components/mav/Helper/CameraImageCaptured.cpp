//
// CameraImageCaptured.cpp
//
// Created on: Apr 26, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Globals.hpp"
#include "Helper/CameraImageCaptured.hpp"
#include "Helper/Common.hpp"
#include <algorithm>
#include <cstring>

namespace Mav {
namespace Hlpr {

CameraImageCaptured CameraImageCaptured::make(decltype(mavlink_camera_image_captured_t::image_index) aImageIndex,
	decltype(mavlink_camera_image_captured_t::capture_result) aCaptureResult, const char *filename)
{
	CameraImageCaptured mavlinkCameraImageCaptured {};

	Hlpr::Cmn::fieldTimeBootMsInit(mavlinkCameraImageCaptured);
	mavlinkCameraImageCaptured.image_index = aImageIndex;
	mavlinkCameraImageCaptured.capture_result = aCaptureResult;
	std::copy_n(filename, std::min<int>(std::strlen(filename), sizeof(mavlinkCameraImageCaptured.file_url)),
		mavlinkCameraImageCaptured.file_url);

	return mavlinkCameraImageCaptured;
}

void CameraImageCaptured::packInto(mavlink_message_t &aMessage)
{
	mavlink_msg_camera_image_captured_encode(Globals::getSysId(), Globals::getCompId(), &aMessage, this);
}

}  // namespace Hlpr
}  // namespace Mav

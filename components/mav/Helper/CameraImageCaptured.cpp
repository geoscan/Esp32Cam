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
#include <limits>

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

CameraImageCaptured CameraImageCaptured::make(decltype(mavlink_camera_image_captured_t::image_index) aImageIndex,
	decltype(mavlink_camera_image_captured_t::capture_result) aCaptureResult, int aNumericName)
{
	static const auto kNameMaxLength = snprintf(nullptr, 0, "%d", std::numeric_limits<decltype(aNumericName)>::max());
	char name[kNameMaxLength + 1] = {0};
	snprintf(name, sizeof(name), "%d", aNumericName);

	return make(aImageIndex, aCaptureResult, name);
}

void CameraImageCaptured::packInto(mavlink_message_t &aMessage)
{
	mavlink_msg_camera_image_captured_encode(Globals::getSysId(), Globals::getCompId(), &aMessage, this);
}

}  // namespace Hlpr
}  // namespace Mav

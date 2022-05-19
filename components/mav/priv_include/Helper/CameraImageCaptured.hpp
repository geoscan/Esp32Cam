//
// CameraImageCaptured.hpp
//
// Created on: Apr 26, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_HELPER_CAMERAIMAGECAPTURED_HPP_
#define MAV_PRIV_INCLUDE_HELPER_CAMERAIMAGECAPTURED_HPP_

#include "Mavlink.hpp"
#include "Common.hpp"

namespace Mav {
namespace Hlpr {

struct CameraImageCaptured : mavlink_camera_image_captured_t, Cmn::Impl::Pack<mavlink_camera_image_captured_t> {
	static CameraImageCaptured make(decltype(mavlink_camera_image_captured_t::image_index) aImageIndex,
		decltype(mavlink_camera_image_captured_t::capture_result) aCaptureResult, const char *fileName);
	static CameraImageCaptured make(decltype(mavlink_camera_image_captured_t::image_index) aImageIndex,
		decltype(mavlink_camera_image_captured_t::capture_result) aCaptureResult, int numericName);
};

}  // namespace Hlpr
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_HELPER_CAMERAIMAGECAPTURED_HPP_

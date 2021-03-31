//
// Frame.hpp
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAM_CAM_FRAME_HPP
#define CAM_CAM_FRAME_HPP

#include "utility/Buffer.hpp"

namespace Cam {

struct Frame : Utility::Buffer {
	using Utility::Buffer::Buffer;
	using Utility::Buffer::operator=;

	virtual int width();
	virtual int height();
};

}  // namespace Cam

#endif  // CAM_CAM_FRAME_HPP

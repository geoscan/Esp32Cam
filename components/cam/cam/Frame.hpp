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

struct Frame : Ut::Buffer {
	using Ut::Buffer::Buffer;
	using Ut::Buffer::operator=;

	virtual int width();
	virtual int height();
	virtual bool valid();
};

}  // namespace Cam

#endif  // CAM_CAM_FRAME_HPP

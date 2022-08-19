//
// Frame.hpp
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAM_CAM_FRAME_HPP
#define CAM_CAM_FRAME_HPP

#include "utility/cont/Buffer.hpp"

namespace Cam {

struct Frame : Ut::Cont::Buffer {
	using Ut::Cont::Buffer::Buffer;
	using Ut::Cont::Buffer::operator=;

	virtual int width();
	virtual int height();
	virtual bool valid();
};

}  // namespace Cam

#endif  // CAM_CAM_FRAME_HPP

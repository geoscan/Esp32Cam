//
// wq.hpp
//
// Created on: Jun 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(WQ_INCLUDE_WQ_HPP_)
#define WQ_INCLUDE_WQ_HPP_

namespace Wq {

static inline constexpr const char *debugTag()
{
	return "[wq]";
}

void start();

}  // namespace Wq

#endif // WQ_INCLUDE_WQ_HPP_

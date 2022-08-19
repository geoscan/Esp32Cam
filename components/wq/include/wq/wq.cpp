//
// wq.cpp
//
// Created on: Jun 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "wq.hpp"
#include "utility/thr/WorkQueue.hpp"

namespace Wq {

void start()
{
	static Ut::Thr::Wq::MediumPriority wqMediumPriority;
	wqMediumPriority.start();
}

}  // namespace Wq

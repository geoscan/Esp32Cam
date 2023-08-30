//
// wq.cpp
//
// Created on: Jun 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/MakeSingleton.hpp"
#include "utility/thr/WorkQueue.hpp"
#include "wq/PortWorkQueue.hpp"

#include "wq.hpp"

namespace Wq {

void start()
{
	static Ut::Thr::Wq::MediumPriority wqMediumPriority{};
	static Wq::PortWorkQueue portWorkQueue{};
	Ut::MakeSingleton<Sys::WorkQueue>::setInstance(portWorkQueue);
	wqMediumPriority.start();
}

}  // namespace Wq

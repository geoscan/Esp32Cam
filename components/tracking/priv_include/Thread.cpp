//
// Thread.cpp
//
// Created on: Oct 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <mosse/portsrc/Port/Task.hpp>
#include "utility/thr/Threading.hpp"
#include "Thread.hpp"

/// \brief Switches sequentially between core 0 and core 1 to achieve physical parallellism
///
static int corePin()
{
	static int sCoreAffinity = 0;
	static constexpr int knCores = 2;
	int ret = sCoreAffinity;
	sCoreAffinity = (sCoreAffinity + 1) % knCores;

	return ret;
}

std::unique_ptr<Mosse::Port::Thread> Trk::Thread::makeFromTask(Mosse::Port::Task &aTask)
{
	return Mosse::Port::Thread::makeUnique<Thread>(aTask);
}

void Trk::Thread::start()
{
	Ut::Thr::Config{true}.core(corePin());  // Configure core pin
	thread = std::unique_ptr<std::thread>{new std::thread{&Mosse::Port::Task::run, task()}};
}

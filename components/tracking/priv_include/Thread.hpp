//
// Thread.hpp
//
// Created on: Oct 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(TRACKING_PRIV_INCLUDE_THREAD_HPP_)
#define TRACKING_PRIV_INCLUDE_THREAD_HPP_

#include <mosse/portsrc/Port/Thread.hpp>
#include <thread>

namespace Trk {

class Thread : public Mosse::Port::Thread {
public:
	std::unique_ptr<Mosse::Port::Thread> makeFromTask(Mosse::Port::Task &) override;
	void start() override;
	~Thread() override;
	using Mosse::Port::Thread::Thread;
private:
	std::unique_ptr<std::thread> thread;
};

}  // namespace Trk

#endif // TRACKING_PRIV_INCLUDE_THREAD_HPP_

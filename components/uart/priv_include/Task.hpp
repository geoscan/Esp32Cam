//
// UartTask.hpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UART_UARTTASK_HPP
#define UART_UARTTASK_HPP

#include <list>
#include <array>
#include <mutex>

namespace Ut {
namespace Sys {
class UartDevice;
}  // namespace Sys
}  // namespace Ut

namespace Ut {

template <class T, std::size_t N>
class CircularSwap;

}  // namespace Ut

namespace Uart {

class Task {
public:
	template <class ...Tuarts>
	Task(Tuarts &&...aArgs);

	void taskRead();
	void taskProcess();

private:
	struct Buf {
		std::array<std::uint8_t, 1024> buf;
		std::size_t pos;
		Ut::Sys::UartDevice *device;
	};

	using Swap = typename Ut::Cont::CircularSwap<Buf, 3>;

private:
	const std::list<Ut::Sys::UartDevice *> uartDevices;

	struct SyncedSwap {
		Swap swap;
		std::mutex mutex;

		Buf *getFree();
		Buf *getFull();
		void pushFree(Buf *);
		void pushFull(Buf *);
	} swap;
};

}  // namespace Uart

#include "Task.impl"

#endif  // UART_UARTTASK_HPP

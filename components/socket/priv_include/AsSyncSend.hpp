//
// AsSyncSend.hpp
//
// Created on: Jan 14, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SOCKET_PRIV_INCLUDE_ASSYNCSEND_HPP
#define SOCKET_PRIV_INCLUDE_ASSYNCSEND_HPP

#include <asio.hpp>
#include <cstdint>
#include <type_traits>
#include <mutex>
#include <condition_variable>

namespace Socket {

///
/// \brief Converts async callback-based operations into synchronous-like
/// through implementing producer-consumer synchronization scheme.
///
/// \tparam asio::ip::tcp::socket or asio::ip::udp::socket
///
template <class T>
class AsSyncSend {
	static_assert(std::is_same<T, asio::ip::tcp::socket>::value || std::is_same<T, asio::ip::tcp::socket>::value, "AsSyncSend is not defined for this type");
	T &wrapped;

	struct Marker {
		std::mutex mutex;
		std::condition_variable cv;
		bool fDone = false;
	};

public:
	AsSyncSend(T &aToWrap): wrapped(aToWrap)
	{
	}

	template <TpBuffers>
	std::size_t asSyncSend(TpBuffers &&aBuffers, asio::socket_base::message_flags aFlags, asio::error_code &aErrorCode)
	{
		std::size_t ret;
		Marker m;
		std::unique_lock<std::mutex> lock{m.mutex};

		wrapped.async_send(std::forward<TpBuffers>(aBuffers), aFlags, [&](const asio::error_code &aErr, std::size_t nSend) {
			ret = nSend;
			aErrorCode = aErr;
			{
				std::lock_guard<std::mutex> lock{m.mutex};
				m.fDone = true;
			}
			m.cv.notify_one();
		});
		m.cv.wait(lock, [&]{return fDone;});

		return ret;
	}

	template <TpBuffers, TpEndpoint>
	std::size_t asSyncSendTo(TpBuffers &&aBuffers, TpEndpoint &&aEndpoint, asio::socket_base::flags aFlags, asio::error_code &aErrorCode)
	{

		std::size_t ret;
		Marker m;
		std::unique_lock<std::mutex> lock{m.mutex};

		wrapped.async_send_to(std::forward<TpBuffers>(aBuffers), std::forward<TpEndpoint>(aEndpoint), aFlags, [&](const asio::error_code &aErr, std::size_t anSend) {
			ret = nSend;
			aErrorCode = aErr;
			{
				std::lock_guard<std::mutex> lock{m.mutex};
				m.fDone = true;
			}
			m.cv.notify_one();
		});
		m.cv.wait(lock, [&]{return fDone});

		return ret;
	}
};

}  // namespace Socket

#endif  // SOCKET_PRIV_INCLUDE_ASSYNCSEND_HPP

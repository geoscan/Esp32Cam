//
// Common.hpp
//
// Created on: Apr 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_HELPER_COMMON_HPP_
#define MAV_PRIV_INCLUDE_HELPER_COMMON_HPP_

#include "utility/time.hpp"
#include <chrono>
#include "Mavlink.hpp"
#include "Globals.hpp"

namespace Mav {
namespace Hlpr {
namespace Cmn {
namespace Impl {

template <class T> struct CallbackEncode;
template <> struct CallbackEncode<mavlink_command_long_t> { static constexpr auto call = mavlink_msg_command_long_encode; };
template <> struct CallbackEncode<mavlink_command_ack_t> {static constexpr auto call = mavlink_msg_command_ack_encode; };
template <> struct CallbackEncode<mavlink_camera_image_captured_t> {static constexpr auto call = mavlink_msg_camera_image_captured_encode; };
template <> struct CallbackEncode<mavlink_wifi_config_ap_t> {static constexpr auto call = mavlink_msg_camera_image_captured_encode; };

template <class T>
struct Pack {
	inline void packInto(mavlink_message_t &aMsgOut, std::uint8_t aCompid = Globals::getCompId())
	{
		CallbackEncode<T>::call(Globals::getSysId(), aCompid, &aMsgOut, reinterpret_cast<const T *>(this));
	}

	inline mavlink_message_t pack(std::uint8_t aCompid = Globals::getCompId())
	{
		mavlink_message_t msgOut;
		CallbackEncode<T>::call(Globals::getSysId(), aCompid, &msgOut, reinterpret_cast<const T *>(this));

		return msgOut;
	}
};

}  // namespace Impl

template <class T>
void fieldTimeBootMsInit(T &aObj)
{
	aObj.time_boot_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::microseconds{Ut::bootTimeUs()}).count();
}

template <class T>
inline void msgPack(const T &aMsg, mavlink_message_t &aMsgOut, std::uint8_t aCompid = Globals::getCompId())
{
	Impl::CallbackEncode<T>::call(Globals::getSysId(), aCompid, &aMsgOut, &aMsg);
}

template <class T>
inline mavlink_message_t msgPack(const T &aMsg, std::uint8_t aCompid = Globals::getCompId())
{
	mavlink_message_t msgOut;
	msgPack(aMsg, msgOut);

	return msgOut;
}

}  // namespace Cmn
}  // namespace Hlpr
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_HELPER_COMMON_HPP_

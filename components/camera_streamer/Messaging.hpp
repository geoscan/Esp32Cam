#ifndef CAMERA_STREAMER_MESSAGING_HPP
#define CAMERA_STREAMER_MESSAGING_HPP

#include "Ov2640.hpp"
#include <utility>
#include <asio.hpp>
#include "utility/Subscription.hpp"

namespace CameraStreamer {

namespace Topic {

struct NewFrame {
};
struct TcpConnected {
};
struct TcpDisconnected {
};
struct WifiDisconnected {
};

}  // namespace Topic

using Port = unsigned short;

namespace Key {

	using TcpConnected     = Rr::Subscription::KeyBase<Topic::TcpConnected, Rr::Subscription::DefaultSyncTraits, asio::ip::address, Port>;
	using NewFrame         = Rr::Subscription::Key<Ov2640::Image &, Topic::NewFrame>;
	using TcpDisconnected  = Rr::Subscription::Key<asio::ip::address, Topic::TcpDisconnected>;
	using WifiDisconnected = Rr::Subscription::Key<asio::ip::address, Topic::WifiDisconnected>;

}  // namespace Key


}  // namespace CameraStreamer

#endif  // CAMERA_STREAMER_MESSAGING_HPP

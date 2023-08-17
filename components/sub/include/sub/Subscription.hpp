#ifndef UTILITY_UTILITY_SUBSCRIPTION_HPP
#define UTILITY_UTILITY_SUBSCRIPTION_HPP

#include "cam/Camera.hpp"
#include "sub/Types.hpp"
#include "utility/cont/Buffer.hpp"
#include <Rr/Util/Key.hpp>
#include <asio.hpp>
#include <list>

namespace Sub {

namespace Topic {
struct NewFrame;
struct TcpConnected;
struct TcpDisconnected;
struct WifiDisconnected;
struct RecordStart;
struct RecordStop;
struct Default;
struct MavlinkUartReceived;
struct MavlinkUdpReceived;
struct MavlinkUartSend;
struct MavlinkUdpSend;
struct MavlinkForward;
struct Ip;
struct IpDisconnect;
}  // namespace Topic

struct MutSyncTrait {
	static constexpr auto kPolicy = Rr::Sync::Policy::Type::Mutex;
	using Mutex = std::mutex;
};

struct NoSyncTrait {
	static constexpr auto kPolicy = Rr::Sync::Policy::Type::None;
};

template <class Tsignature, class Ttopic = Topic::Default>
using IndKey = Rr::Util::Key<Tsignature, MutSyncTrait, std::list, Ttopic>;

template <class Tsignature, class Ttopic = Topic::Default>
using NoLockKey = Rr::Util::Key<Tsignature, NoSyncTrait, std::list, Ttopic>;

namespace Key {

using NewFrameEvent = const typename std::shared_ptr<Cam::Frame> &;

// Obsolete Rr::Subscription::Key
using TcpConnected     = IndKey<void(asio::ip::address, Port), Topic::TcpConnected>;
using NewFrame         = IndKey<void(NewFrameEvent), Topic::NewFrame>;  // TODO: considering frame rate, using `shared_ptr` is not a very smart move: mind the allocations it entails
using TcpDisconnected  = IndKey<void(asio::ip::address), Topic::TcpDisconnected>;
using WifiDisconnected = IndKey<void(asio::ip::address), Topic::WifiDisconnected>;

}  // namespace Key
}  // namespace Sub

#endif // UTILITY_UTILITY_SUBSCRIPTION_HPP

#ifndef UTILITY_UTILITY_SUBSCRIPTION_HPP
#define UTILITY_UTILITY_SUBSCRIPTION_HPP

#include <Rr/Util/Key.hpp>
#include "utility/cont/Buffer.hpp"
#include "cam/Camera.hpp"
#include "sub/Types.hpp"
#include <asio.hpp>
#include <list>

namespace Rr {
namespace Subscription {

struct MutSyncTrait {
	static constexpr auto kPolicy = Rr::Sync::Policy::Type::Mutex;
	using Mutex = std::mutex;
};

using DefaultSyncTrait = MutSyncTrait;

template <class Targ, class Ttopic>
using Key = typename Rr::Util::LegacyKey<void(Targ), DefaultSyncTrait, std::list, Ttopic>;

template <class Ttopic>
using KeyVoid = typename Rr::Util::LegacyKey<void(), DefaultSyncTrait, std::list, Ttopic>;

template <class Ttopic, class TsyncTrait, class ...Targs>
using KeyBase = typename Rr::Util::LegacyKey<void(Targs...), DefaultSyncTrait, std::list, Ttopic>;

}  // namespace Subscription
}  // namespace Rr

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

// Obsolete Rr::Subscription::Key
using TcpConnected     = Rr::Subscription::KeyBase<Topic::TcpConnected, Rr::Subscription::DefaultSyncTrait, asio::ip::address, Port>;
using NewFrame         = Rr::Subscription::Key<const std::shared_ptr<Cam::Frame> &, Topic::NewFrame>;
using TcpDisconnected  = Rr::Subscription::Key<asio::ip::address, Topic::TcpDisconnected>;
using WifiDisconnected = Rr::Subscription::Key<asio::ip::address, Topic::WifiDisconnected>;

}  // namespace Key
}  // namespace Sub

#endif // UTILITY_UTILITY_SUBSCRIPTION_HPP

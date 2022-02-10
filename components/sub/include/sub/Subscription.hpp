#ifndef UTILITY_UTILITY_SUBSCRIPTION_HPP
#define UTILITY_UTILITY_SUBSCRIPTION_HPP

#include <Rr/Subscription.hpp>
#include <Rr/Key.hpp>
#include <Rr/Module.hpp>
#include <Rr/SyncTrait.hpp>
#include "utility/Buffer.hpp"
#include "cam/Camera.hpp"
#include "sub/Types.hpp"
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

///
/// \brief Synchronization group
///
enum class SyncGroup : unsigned {
	MavlinkRouting = 0,
};

template <class Tsignature, SyncGroup Igroup, class Ttopic = Topic::Default>
using GroupKey = Rr::Key<Tsignature, Ttopic, std::list, Rr::DefaultGroupMutexTrait<(unsigned)Igroup>>;

template <class Tsignature, SyncGroup Igroup, class Ttopic = Topic::Default>
using GroupModule = Rr::Module<Tsignature, Ttopic, std::list, Rr::DefaultGroupMutexTrait<(unsigned)Igroup>>;

template <SyncGroup Igroup>
using GroupLock = Rr::GroupMutexLock<Rr::DefaultGroupMutexTrait<unsigned(Igroup)>>;

template <class Tsignature, class Ttopic = Topic::Default>
using IndKey = Rr::Key<Tsignature, Ttopic, std::list, Rr::DefaultMutexTrait>;

template <class Tsignature, class Ttopic = Topic::Default>
using IndModule = Rr::Module<Tsignature, Ttopic, std::list, Rr::DefaultMutexTrait>;

template <class Tsignature, class Ttopic = Topic::Default>
using NoLockKey = Rr::Key<Tsignature, Ttopic, std::list, Rr::MockMutexTrait>;

template <class Tsignature, class Ttopic = Topic::Default>
using NoLockModule = Rr::Module<Tsignature, Ttopic, std::list, Rr::MockMutexTrait>;

namespace Key {

// Obsolete Rr::Subscription::Key
using TcpConnected     = Rr::Subscription::KeyBase<Topic::TcpConnected, Rr::Subscription::DefaultSyncTrait, asio::ip::address, Port>;
using NewFrame         = Rr::Subscription::Key<const std::shared_ptr<Cam::Frame> &, Topic::NewFrame>;
using TcpDisconnected  = Rr::Subscription::Key<asio::ip::address, Topic::TcpDisconnected>;
using WifiDisconnected = Rr::Subscription::Key<asio::ip::address, Topic::WifiDisconnected>;
using RecordStart      = Rr::Subscription::Key<const std::string &/*filename*/, Topic::RecordStart>;
using RecordStop       = Rr::Subscription::Key<void, Topic::RecordStop>;

// As for 2021-12-23, a duplex 1-to-1 version of forwarding is used. No locking is required
using MavlinkUartReceived = NoLockModule<MavReceiveResult(Message &), Topic::MavlinkUdpReceived>;
using MavlinkUdpReceived = NoLockModule<MavReceiveResult(Message &), Topic::MavlinkUartReceived>;
using MavlinkUdpSend = NoLockModule<IpSendResult(Message &), Topic::MavlinkUdpSend>;
using MavlinkUartSend = NoLockModule<UartSendResult(Message &), Topic::MavlinkUartSend>;  /// Will be implemented as a wrapper over IpSend key

using IpConnect = IndModule<IpResult(const Sub::IpConnect &)>;  ///< Fullfill a TCP connection request
using IpSend = IndModule<IpSendResult(const IpDestMessage &)>;  ///< Send an IP package
using IpReceived = IndModule<void(const IpDestMessage &)>;  ///< Handle IP received data

using UartSend = IndModule<UartSendResult(const UartMessage &)>;  ///< Send a package over serial

}  // namespace Key
}  // namespace Sub

#endif // UTILITY_UTILITY_SUBSCRIPTION_HPP

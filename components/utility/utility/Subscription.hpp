#ifndef UTILITY_UTILITY_SUBSCRIPTION_HPP
#define UTILITY_UTILITY_SUBSCRIPTION_HPP

#include <Rr/Subscription.hpp>
#include <Rr/Key.hpp>
#include <Rr/Module.hpp>
#include <Rr/SyncTrait.hpp>
#include "Buffer.hpp"
#include "cam/Camera.hpp"
#include <asio.hpp>
#include <list>

namespace Utility {
namespace Subscription {

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

// Various types pertaining to subscription mechanisms

using Port = std::uint16_t;
using UartNum = std::uint8_t;

struct IpEndpoint {
	std::uint8_t address[4];
	Port port;
};

struct IpEndpointHost {
	Port hostPort;
};

struct UartEndpoint {
	UartNum uartNum;
};

enum class EndpointType {
	Tcp,
	Udp,
	Uart,
	Unspecified,
};

struct Message {
	Utility::ConstBuffer payload;
};

struct UartMessage : Message, UartEndpoint {
};

struct IpMessage : Message, IpEndpoint {
	enum IpTransport {
		Tcp,
		Udp,
	} transport;
};

struct IpDestMessage : IpMessage, IpEndpointHost {
};

using RoutingResult = void;  // Extension point

namespace Key {

// Obsolete Rr::Subscription::Key
using TcpConnected     = Rr::Subscription::KeyBase<Topic::TcpConnected, Rr::Subscription::DefaultSyncTrait, asio::ip::address, Port>;
using NewFrame         = Rr::Subscription::Key<const std::shared_ptr<Cam::Frame> &, Topic::NewFrame>;
using TcpDisconnected  = Rr::Subscription::Key<asio::ip::address, Topic::TcpDisconnected>;
using WifiDisconnected = Rr::Subscription::Key<asio::ip::address, Topic::WifiDisconnected>;
using RecordStart      = Rr::Subscription::Key<const std::string &/*filename*/, Topic::RecordStart>;
using RecordStop       = Rr::Subscription::Key<void, Topic::RecordStop>;

// As for 2021-12-23, a duplex 1-to-1 version of forwarding is used. No locking is required
using MavlinkUartReceived = NoLockKey<RoutingResult(Message &), Topic::MavlinkUdpReceived>;
using MavlinkUdpReceived = NoLockKey<RoutingResult(Message &), Topic::MavlinkUartReceived>;
using MavlinkUdpSend = NoLockKey<RoutingResult(Message &), Topic::MavlinkUdpSend>;
using MavlinkUartSend = NoLockKey<RoutingResult(Message &), Topic::MavlinkUartSend>;

}  // namespace Key

}  // namespace Subscription
}  // namespace Utility

#endif // UTILITY_UTILITY_SUBSCRIPTION_HPP

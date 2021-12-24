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
struct IpConnect;
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

// Various typedefs pertaining to subscription mechanisms

using Port = std::uint16_t;  ///< TCP / UDP port
using UartNum = std::uint8_t;  ///< Id. of UART interface
using Ip4 = std::uint8_t[4];  ///< IPv4 Address

// Atomic entities serving as shortcuts for concrete types used in routing and
// different control commands.

struct IpTransport {
	enum {
		Tcp,
		Udp,
	} transport;  ///< A type of IP transport that is being used
};

struct IpEndpoint {
	Ip4 address;  ///< Remote IP address
	Port port;  ///< Remote IP port
};

struct IpEndpointHost {
	Port hostPort;  ///< Local IP port
};

struct UartEndpoint {
	UartNum uartNum;  ///< Id. of UART interface being used
};

struct Message {
	Utility::ConstBuffer payload;  ///< Raw bytes transferred with an interface
};

// Derivatives of the atomic entities defined above

///
/// \brief In/Out message. Message buffer + info on UART used
///
struct UartMessage : Message, UartEndpoint {
};

///
/// \brief Incoming message
///
struct IpMessage : Message, IpEndpoint, IpTransport {
};

///
/// \brief Outgoing message
///
struct IpDestMessage : IpMessage, IpEndpointHost {
};

///
/// \brief Socket control operations
///
struct IpDestEndpoint : IpEndpoint, IpEndpointHost {
};

///
/// \brief Encapsulation of a command evaluated upon an IP interface
///
struct IpConnect : IpDestEndpoint {
	bool connect;  ///< true for connect, false for disconnect
};

// Routing and control result types

enum class Result {
	None = -1,  ///< A request cannot be fulfilled by this particular provider
	Ok = 0,
	Fail,

	Max,
};

///
/// \brief Encapsulates all the necessary information acquired from making an
/// IP request such as connect or disconnect. Nesting is used to provide
/// painless extension without changing too much code, if such will be rendered
/// necessary in the future.
///
struct IpResult {
	Result result;  ///< Generic result.
};

struct RoutingResult {
};

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

using IpConnect = IndModule<IpResult(const IpConnect &), Topic::IpConnect>;  ///<

}  // namespace Key

}  // namespace Subscription
}  // namespace Utility

#endif // UTILITY_UTILITY_SUBSCRIPTION_HPP

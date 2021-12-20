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
}  // namespace Topic

///
/// \brief Synchronization group
///
enum class SyncGroup : unsigned {
	MavlinkRouting = 0,
};

template <class Tsignature, SyncGroup Igroup, class Ttopic = Topic::Default>
using GroupModule = Rr::Module<Tsignature, Ttopic, std::list, Rr::DefaultGroupMutexTrait<(unsigned)Igroup>>;

template <class Tsignature, SyncGroup Igroup, class Ttopic = Topic::Default>
using GroupKey = Rr::Key<Tsignature, Ttopic, std::list, Rr::DefaultGroupMutexTrait<(unsigned)Igroup>>;

template <SyncGroup Igroup>
using GroupLock = Rr::GroupMutexLock<Rr::DefaultGroupMutexTrait<unsigned(Igroup)>>;

// Various types pertaining to subscription mechanisms

using Port = unsigned short;

struct Routing {
	enum class Endpoint {
		MavlinkUart,
		MavlinkUdp,
		TrikTcp,
		Other,
	};
	Endpoint src;
	Endpoint dest;
	Utility::ConstBuffer buffer;
};

namespace Key {

// Obsolete Rr::Subscription::Key
using TcpConnected     = Rr::Subscription::KeyBase<Topic::TcpConnected, Rr::Subscription::DefaultSyncTrait, asio::ip::address, Port>;
using NewFrame         = Rr::Subscription::Key<const std::shared_ptr<Cam::Frame> &, Topic::NewFrame>;
using TcpDisconnected  = Rr::Subscription::Key<asio::ip::address, Topic::TcpDisconnected>;
using WifiDisconnected = Rr::Subscription::Key<asio::ip::address, Topic::WifiDisconnected>;
using RecordStart      = Rr::Subscription::Key<const std::string &/*filename*/, Topic::RecordStart>;
using RecordStop       = Rr::Subscription::Key<void, Topic::RecordStop>;

using MavlinkRouting = GroupModule<bool(Routing &), SyncGroup::MavlinkRouting>;

}  // namespace Key

}  // namespace Subscription
}  // namespace Utility

#endif // UTILITY_UTILITY_SUBSCRIPTION_HPP

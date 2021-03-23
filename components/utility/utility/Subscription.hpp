#ifndef UTILITY_UTILITY_SUBSCRIPTION_HPP
#define UTILITY_UTILITY_SUBSCRIPTION_HPP

#include <rr/subscription/subscription.hpp>
#include <asio.hpp>
#include "Ov2640.hpp"

namespace Utility {
namespace Subscription {

namespace Topic {

struct NewFrame {
};
struct TcpConnected {
};
struct TcpDisconnected {
};
struct WifiDisconnected {
};
struct RecordStart {
};
struct RecordStop {
};

}  // namespace Topic

using Port = unsigned short;

namespace Key {

	using TcpConnected     = Rr::Subscription::KeyBase<Topic::TcpConnected, Rr::Subscription::DefaultSyncTraits, asio::ip::address, Port>;
	using NewFrame         = Rr::Subscription::Key<const std::shared_ptr<Ov2640::Image> &, Topic::NewFrame>;
	using TcpDisconnected  = Rr::Subscription::Key<asio::ip::address, Topic::TcpDisconnected>;
	using WifiDisconnected = Rr::Subscription::Key<asio::ip::address, Topic::WifiDisconnected>;
	using RecordStart      = Rr::Subscription::Key<const std::string &/*filename*/, Topic::RecordStart>;
	using RecordStop       = Rr::Subscription::Key<void, Topic::RecordStop>;
}  // namespace Key

}  // namespace Subscription
}  // namespace Utility

#endif // UTILITY_UTILITY_SUBSCRIPTION_HPP
//
// ApVer.hpp
//
// Created on: Jun 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MAV_PRIV_INCLUDE_MICROSERVICE_APVER_HPP_)
#define MAV_PRIV_INCLUDE_MICROSERVICE_APVER_HPP_

#include "Microservice.hpp"
#include "module/ModuleBase.hpp"
#include <cstdint>

namespace Mav {
namespace Mic {

/// \brief Listens for incoming MAVLink HEARTBEAT(0) messages w/ `compid` corresponding to AP board. When one of those
/// is received, requests version info from the AP board
///
class ApVer : public Microservice, public Mod::ModuleBase {
private:
	/// \brief Buffers version info received over MAVLink
	///
	struct Version {
		std::uint8_t swMajor;  ///< Software version, major revision
		std::uint8_t swMinor;  ///< Software version, minor revision
		std::uint32_t swRev;  ///< Software version, patch revision, or the number of commits from the latest revision,
		std::uint32_t swCommit;  ///< Git commit hash
	};

	/// \brief Stores communication state
	///
	struct Updated {
		bool version;  ///< `true` means AUTOPILOT_VERSION(148) message has been received
	};

public:
	ApVer();
	Microservice::Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse);

protected:
	void getFieldValue(Mod::Fld::Req aReq, Mod::Fld::OnResponseCallback aOnResponse) override;

private:
	Version version;
	Updated updated;
};

}  // namespace Mic
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_APVER_HPP_

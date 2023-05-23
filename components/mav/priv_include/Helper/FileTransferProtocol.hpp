//
// FileTransferProtocol.hpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_MAV_PRIV_INCLUDE_HELPER_FILETRANSFERPROTOCOL_HPP
#define COMPONENTS_MAV_PRIV_INCLUDE_HELPER_FILETRANSFERPROTOCOL_HPP

#include "Common.hpp"
#include "Mavlink.hpp"
#include "Ftp/Types.hpp"

namespace Mav {
namespace Hlpr {

struct FileTransferProtocol : mavlink_file_transfer_protocol_t, Cmn::Impl::Pack<mavlink_file_transfer_protocol_t> {
	/// \brief C MAVLink implementation does not allow parsing `payload` field
	/// of `FILE_TRANSFER_PROTOCOL` message. `getPayload` fixes the problem
	inline Mav::Mic::Ftp::Payload &getPayload()
	{
		return *reinterpret_cast<Mav::Mic::Ftp::Payload *>(&payload);
	}
};

}  // namespace Hlpr
}  // namespace Mav

#endif // COMPONENTS_MAV_PRIV_INCLUDE_HELPER_FILETRANSFERPROTOCOL_HPP

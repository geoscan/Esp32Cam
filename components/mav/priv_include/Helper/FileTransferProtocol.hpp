//
// FileTransferProtocol.hpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_MAV_PRIV_INCLUDE_HELPER_FILETRANSFERPROTOCOL_HPP
#define COMPONENTS_MAV_PRIV_INCLUDE_HELPER_FILETRANSFERPROTOCOL_HPP

#include "Common.hpp"
#include "Globals.hpp"
#include "Mavlink.hpp"
#include "Microservice/Ftp/Types.hpp"
#include <algorithm>
#include <cstdint>

namespace Mav {
namespace Hlpr {

struct FileTransferProtocol : mavlink_file_transfer_protocol_t, Cmn::Impl::Pack<mavlink_file_transfer_protocol_t> {
	/// \brief C MAVLink implementation does not allow parsing `payload` field
	/// of `FILE_TRANSFER_PROTOCOL` message. `getPayload` fixes the problem
	inline Mav::Mic::Ftp::Payload &getPayload()
	{
		return *reinterpret_cast<Mav::Mic::Ftp::Payload *>(&payload);
	}

	inline const Mav::Mic::Ftp::Payload &getPayload() const
	{
		return *reinterpret_cast<const Mav::Mic::Ftp::Payload *>(&payload);
	}

	/// \brief Fields related to creating a file session.
	///	None = 0.
	///	\param `aOperation`. Expected values are: `OpenFileRo`, `CreateFile`,
	///	`OpenFileWo`
	inline void setOpenFileSessionFields(std::uint16_t aSequenceNumber, Mic::Ftp::Op aOperation,
		const char *aFilePath, std::size_t aFilePathLength, std::uint8_t aTargetSystem = Globals::getCompidAutopilot(),
		std::uint8_t aTargetComponent = Globals::getSysId())
	{
		target_system = aTargetSystem;
		target_component = aTargetComponent;
		target_network = 0;
		getPayload().seq_number = aSequenceNumber;
		getPayload().opcode = aOperation;
		std::copy_n(aFilePath, aFilePathLength, getPayload().data);
		getPayload().size = aFilePathLength;
		// TODO: XXX: should it also set `req_opcode` to None?
	}

	/// \pre `aDataSize` must not exceed 239, which is the maximum payload length
	inline void setWriteFileFields(std::uint16_t aSequenceNumber, Mic::Ftp::SessionId aSessionId,
		std::uint32_t aFileOffset, std::uint8_t aTargetSystem = Globals::getCompidAutopilot(),
		std::uint8_t aTargetComponent = Globals::getSysId(), std::uint8_t *aData = nullptr,
		Mic::Ftp::Size aDataSize = 0)
	{
		target_system = aTargetSystem;
		target_component = aTargetComponent;
		target_network = 0;
		getPayload().seq_number = aSequenceNumber;
		getPayload().session = aSessionId;
		getPayload().size = aDataSize;
		getPayload().offset = aFileOffset;
		getPayload().opcode = Mic::Ftp::Op::WriteFile;

		if (aData != nullptr && aDataSize > 0) {
			std::copy_n(aData, aDataSize, getPayload().data);
		}
		// TODO: XXX: should it also set `req_opcode` to None?
	}

	inline void setTerminateSessionFields(std::uint16_t aSequenceNumber, Mic::Ftp::SessionId aSessionId,
		std::uint8_t aTargetSystem = Globals::getCompidAutopilot(), std::uint8_t aTargetComponent = Globals::getSysId())
	{
		target_system = aTargetSystem;
		target_component = aTargetComponent;
		target_network = 0;
		getPayload().seq_number = aSequenceNumber;
		getPayload().session = aSequenceNumber;
		getPayload().opcode = Mic::Ftp::Op::TerminateSession;
	}
};

}  // namespace Hlpr
}  // namespace Mav

#endif // COMPONENTS_MAV_PRIV_INCLUDE_HELPER_FILETRANSFERPROTOCOL_HPP

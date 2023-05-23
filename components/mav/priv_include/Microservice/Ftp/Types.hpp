//
// Types.hpp
//
// Created on: Aug 24, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MODULES_MESSAGING_MAVLINK_MICROSERVICES_MICFTP_TYPES_HPP_
#define MODULES_MESSAGING_MAVLINK_MICROSERVICES_MICFTP_TYPES_HPP_

#include <cstdint>
#include <string>

namespace Mav {
namespace Mic {
namespace Ftp {

///
/// \brief MAVLink FTP operation IDs. Refer to
/// https://mavlink.io/en/services/ftp.html#opcodes for more info
///
enum class Op : uint8_t {
	None = 0,
	TerminateSession,
	ResetSessions,
	ListDirectory,
	OpenFileRo,
	ReadFile,
	CreateFile,
	WriteFile,
	RemoveFile,
	CreateDirectory,
	RemoveDirectory,
	OpenFileWo,
	TruncateFile,
	Rename,
	CalcFileCrc32,
	BurstReadFile,
	Ack = 128,
	Nak
};

/// \enum Nak
///
/// \brief MAVLink FTP error response codes. Refer to
/// https://mavlink.io/en/services/ftp.html#error_codes for more info
///
enum class Nak : uint8_t {
	None = 0,
	Fail,
	FailErrno,
	InvalidDataSize,
	InvalidSession,
	NoSessionsAvailable,
	Eof,
	UnknownCommand,
	FileExists,
	FileProtected,
	FileNotFound
};

/// \struct Payload
/// \brief helper struct for interpreting MAVLink FTP payload
///
struct Payload {
	static constexpr std::size_t kMaxDataLength = 251 - 12;  ///< https://mavlink.io/en/services/ftp.html#payload
	std::uint16_t seq_number;            ///< sequence number for message
	std::uint8_t  session;               ///< Session id for read and write commands
	Op            opcode;                ///< Command opcode
	std::uint8_t  size;                  ///< Size of data
	Op            req_opcode;            ///< Request opcode returned in kRspAck, kRspNak message
	std::uint8_t  burst_complete;        ///< Only used if req_opcode=kCmdBurstReadFile - 1: set of burst packets complete, 0: More burst packets coming.
	std::uint8_t  padding;               ///< 32 bit aligment padding
	std::uint32_t offset;                ///< Offsets for List and Read commands
	std::uint8_t  data[kMaxDataLength];  ///< command data, varies with regard to `opcode` value
} __attribute__((packed));

using SessionId = decltype(Payload::session);
using Offset = decltype(Payload::offset);
using Size = decltype(Payload::size);

}  // namespace Ftp
}  // namespace Mic
}  // namespace Mav

#endif  // MODULES_MESSAGING_MAVLINK_MICROSERVICES_MICFTP_TYPES_HPP_

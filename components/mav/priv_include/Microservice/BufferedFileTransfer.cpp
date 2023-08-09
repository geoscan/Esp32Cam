//
// BufferedFileTransfer.cpp
//
// Created on: Aug 02, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#include "Globals.hpp"
#include "Helper/MavlinkCommandAck.hpp"
#include "buffered_file_transfer/BufferedFileTransfer.hpp"
#include "buffered_file_transfer/Sub.hpp"
#include "http/client/file.h"
#include "mav/mav.hpp"
#include <cstdint>
#include <cstring>
#include <esp_log.h>

#include "BufferedFileTransfer.hpp"

namespace Mav {
namespace Mic {

static constexpr const char *kHttpPreamble = "192.168.4.1:9000";  // TODO: make it configurable from Kconfig
static constexpr unsigned kMaxUrlLength = 128;
static constexpr const char *kLogPreamble = "Mav::Mic::BufferedFileTransfer";
static constexpr MAV_CMD kMavlinkCommandFetchFile = MAV_CMD_USER_2;


/// Encodes a URL. The code implies knowledge of the base station's REST API
/// \pre `aMavlinkMessage` is a MAVLINK_COMMAND_LONG type, with `command`
/// field=MAV_CMD_USER_2, and `sysid`, `compid` set accordingly (see `Globals`)
static bool tryEncodeRequestUrl(char *aBuffer, std::size_t aBufferSize, const mavlink_message_t &aMavlinkMessage);

/// Decomposes message -- returns drone ID
/// \pre `aMavlinkMessage` is a MAVLINK_COMMAND_LONG type, with `command`
/// field=MAV_CMD_USER_2, and `sysid`, `compid` set accordingly (see `Globals`)
static int mavlinkMessageFetchFileGetFileId(const mavlink_message_t &aMavlinkMessage);

Mav::Microservice::Ret BufferedFileTransfer::process(mavlink_message_t &aMavlinkMessage,
	OnResponseSignature aOnResponse)
{
	auto result = Ret::Ignored;
	// TODO: filter repeating requests

	switch (aMavlinkMessage.msgid) {
		case MAVLINK_MSG_ID_COMMAND_LONG: {
			if (mavlink_msg_command_long_get_target_component(&aMavlinkMessage) == Globals::getCompId()
					&& mavlink_msg_command_long_get_target_system(&aMavlinkMessage) == Globals::getSysId()) {
				switch (mavlink_msg_command_long_get_command(&aMavlinkMessage)) {
					case kMavlinkCommandFetchFile:
						result = onCommandLongFetchFile(aMavlinkMessage, aOnResponse);

						break;

					default:
						break;
				}
			}

			break;
		}

		default:
			break;
	}

	return result;
}

inline Microservice::Ret BufferedFileTransfer::onCommandLongFetchFile(mavlink_message_t &aMavlinkMessage,
	OnResponseSignature aOnResponse)
{
	char url[kMaxUrlLength] = {0};

	// Encode URL to fetch the file
	if (!tryEncodeRequestUrl(url, kMaxUrlLength, aMavlinkMessage)) {
		ESP_LOGE(Mav::kDebugTag, "%s:%s: failed to encode URL, not enough buffer space, sending fail response",
			kLogPreamble, __func__);
		auto commandAckFail = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMavlinkMessage, kMavlinkCommandFetchFile,
			MAV_RESULT_FAILED);
		commandAckFail.packInto(aMavlinkMessage);
		aOnResponse(aMavlinkMessage);

		return Ret::Response;
	}

	// Access BFT, open a file, handle allocation errors, if there's any
	if (!Bft::BufferedFileTransfer::checkInstance()) {
		ESP_LOGE(Mav::kDebugTag,
			"%s::%s: a `BufferedFileTransfer` instance is not registered, cannot resume, sending fail response",
			kLogPreamble, __func__);
		auto commandAckFail = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMavlinkMessage, kMavlinkCommandFetchFile,
			MAV_RESULT_FAILED);
		commandAckFail.packInto(aMavlinkMessage);
		aOnResponse(aMavlinkMessage);

		return Ret::Response;
	}

	// TODO: allocate the buffer after receiving content length header

	HttpDownloadContext httpDownloadContext{*this};
	const auto httpDownloadEspErr = httpDownloadFileOverHttpGetByUrl(url, onHttpFileDownloadChunk,
		static_cast<void *>(&httpDownloadContext));

	if (httpDownloadEspErr != ESP_OK) {
		ESP_LOGE(Mav::kDebugTag, "%s::%s: failed to receive file", kLogPreamble, __func__);
		auto commandAckFail = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMavlinkMessage, kMavlinkCommandFetchFile,
			MAV_RESULT_FAILED);
		commandAckFail.packInto(aMavlinkMessage);
		aOnResponse(aMavlinkMessage);

		return Ret::Response;
	}

	// Trigger event notification
	Bft::OnFileBufferingFinished::notify(makeCustomDeallocationSharedFilePointer());

	// Send a response
	auto commandAckFail = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMavlinkMessage, kMavlinkCommandFetchFile,
		MAV_RESULT_ACCEPTED);
	commandAckFail.packInto(aMavlinkMessage);
	aOnResponse(aMavlinkMessage);

	return Ret::Response;
}

esp_err_t BufferedFileTransfer::onHttpFileDownloadChunk(const char *aChunk, std::size_t aChunkSize, void *aData)
{
	esp_err_t espErr = ESP_OK;

	if (aChunk == nullptr) {  // Check if this call is the announcement of the expected binary length (see `http` API). Knowing the expected length is crucial for allocating enough space
		if (aChunkSize > 0) {
			espErr = HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoReceiving(aChunkSize);

			if (espErr != ESP_OK) {  // Cannot transfer into `Receiving` state. Most likely, unable to allocate memory
				ESP_LOGE(Mav::kDebugTag,
					"%s::%s: failed to transfer into receiving state, error=%d, stopping the process",
					kLogPreamble, __func__, static_cast<int>(espErr));
				HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoIdle();
			}
		} else {  // Wrong arguments: file size=0
			ESP_LOGE(Mav::kDebugTag, "%s::%s: invalid announced file size=0", kLogPreamble, __func__);
			HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoIdle();
			espErr = ESP_FAIL;
		}
	} else if (aChunkSize == 0 ) {  // Check if the transmission has been completed
		HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoIdle();
		espErr = ESP_OK;
	} else {  // Regular transmission
		espErr = HttpDownloadContext::castFromVoid(aData).owner.state.handleFileChunk(aChunk, aChunkSize);

		if (espErr != ESP_OK) {
			HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoIdle();
			ESP_LOGE(Mav::kDebugTag, "%s::%s Unable to handle file chunk. Stopping", kLogPreamble, __func__);
		}
	}

	return espErr;
}

static inline bool tryEncodeRequestUrl(char *aBuffer, std::size_t aBufferSize,
		const mavlink_message_t &aMavlinkMessage)
{
	const auto droneId = mavlinkMessageFetchFileGetFileId(aMavlinkMessage);
	const int result = snprintf(aBuffer, aBufferSize, "%s/%d.bin", kHttpPreamble, static_cast<int>(droneId));

	if (result < 0 || result >= aBufferSize) {
		ESP_LOGE(Mav::kDebugTag, "%s::%s: unable to encode URL, not enough memory (buffer size=%d)", kLogPreamble,
			__func__, static_cast<int>(aBufferSize));

		return false;
	}

	return true;
}

static inline int mavlinkMessageFetchFileGetFileId(const mavlink_message_t &aMavlinkMessage)
{
	return static_cast<int>(mavlink_msg_command_long_get_param1(&aMavlinkMessage));
}

}  // Mic
}  // Mav

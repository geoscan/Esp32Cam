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
#include <esp_log.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "BufferedFileTransfer.hpp"

namespace Mav {
namespace Mic {

static constexpr const char *kHttpPreamble = "192.168.4.1:9000";  // TODO: make it configurable from Kconfig
static constexpr unsigned kMaxUrlLength = 128;
static constexpr unsigned kFileNameMaxLength = 32;
static constexpr const char *kLogPreamble = "Mav::Mic::BufferedFileTransfer";
static constexpr MAV_CMD kMavlinkCommandFetchFile = MAV_CMD_USER_2;

/// Encapsulates the knowledge about the AP's internal files
struct AutopilotFile {
	enum {
		/// Script for a light show drone
		LuaShow = 0,

		/// Meta
		N,
	};

	/// Converts AP file id into MAVLink API-compatible file name
	static const char *tryGetNameById(int aAutopilotFileId);
};

/// Encodes a URL. The code implies knowledge of the base station's REST API
/// \pre `aMavlinkMessage` is a MAVLINK_COMMAND_LONG type, with `command`
/// field=MAV_CMD_USER_2, and `sysid`, `compid` set accordingly (see `Globals`)
static bool tryEncodeRequestUrl(char *aBuffer, std::size_t aBufferSize, const mavlink_message_t &aMavlinkMessage);

/// Decomposes message -- returns drone ID
/// \pre `aMavlinkMessage` is a MAVLINK_COMMAND_LONG type, with `command`
/// field=MAV_CMD_USER_2, and `sysid`, `compid` set accordingly (see `Globals`)
static int mavlinkMessageFetchFileGetFileId(const mavlink_message_t &aMavlinkMessage);

/// \brief Parses a MAVLink message, and prints the file name requested by it
/// into the provided buffer
///
/// \return false, if the buffer is too small
/// \pre aMavlinkMessage is of type MAV_COMMAND_LONG
static bool tryEncodeAutopilotFileName(char *aBuffer, std::size_t aBufferSize, const mavlink_message_t &aMavlinkMessage);

/// \brief Boilerplate reducer. Produces log output, packs response, invokes
/// the callback it's been provided with.
static void handlePackResponse(mavlink_message_t &aMavlinkMessage, Microservice::OnResponseSignature &aOnResponse,
	MAV_RESULT aMavResult, esp_log_level_t aEspLogLevel, const char *aContext, const char *aMessage);

Mav::Microservice::Ret BufferedFileTransfer::process(mavlink_message_t &aMavlinkMessage,
	OnResponseSignature aOnResponse)
{
	auto result = Ret::Ignored;

	if (mavlinkMessageIsRepeatingMavCommandLong(aMavlinkMessage)) {
		ESP_LOGW(Mav::kDebugTag, "%s::%s Got repeating request, ignoring", kLogPreamble, __func__);

		return Ret::NoResponse;
	}

	switch (aMavlinkMessage.msgid) {
		case MAVLINK_MSG_ID_COMMAND_LONG: {
			if (mavlink_msg_command_long_get_target_component(&aMavlinkMessage) == Globals::getCompId()
					&& mavlink_msg_command_long_get_target_system(&aMavlinkMessage) == Globals::getSysId()) {
				switch (mavlink_msg_command_long_get_command(&aMavlinkMessage)) {
					case kMavlinkCommandFetchFile:
						result = onCommandLongFetchFile(aMavlinkMessage, aOnResponse);

						break;

					default:
						ESP_LOGD(Mav::kDebugTag, "%s::%s unsupported MAV_COMMAND_LONG.command=%d", kLogPreamble,
							__func__, mavlink_msg_command_long_get_command(&aMavlinkMessage));
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
		handlePackResponse(aMavlinkMessage, aOnResponse, MAV_RESULT_FAILED, esp_log_level_t::ESP_LOG_ERROR, __func__,
			"failed to encode URL, not enough buffer space, sending fail response");

		return Ret::Response;
	}

	// Access BFT, open a file, handle allocation errors, if there's any
	if (!Bft::BufferedFileTransfer::checkInstance()) {
		// TODO: refactor into a separate boilerplate handler
		handlePackResponse(aMavlinkMessage, aOnResponse, MAV_RESULT_FAILED, esp_log_level_t::ESP_LOG_ERROR, __func__,
			"`BufferedFileTransfer` instance is not registered, cannot resume, sending fail response");

		return Ret::Response;
	}

	HttpDownloadContext httpDownloadContext{*this};

	// Handle state machine transfer
	char fileName[kFileNameMaxLength];  // TODO kFileN...
	if (!tryEncodeAutopilotFileName(fileName, kFileNameMaxLength, aMavlinkMessage)) {
		handlePackResponse(aMavlinkMessage, aOnResponse, MAV_RESULT_FAILED, esp_log_level_t::ESP_LOG_ERROR, __func__,
			"cannot encode file name, sending fail response");

		return Ret::Response;
	}

	state.transferIntoHttpInitial(fileName);

	// Run file reception
	const auto httpDownloadEspErr = httpDownloadFileOverHttpGetByUrl(url, onHttpFileDownloadChunk,
		static_cast<void *>(&httpDownloadContext));

	if (httpDownloadEspErr != ESP_OK) {
		handlePackResponse(aMavlinkMessage, aOnResponse, MAV_RESULT_FAILED, esp_log_level_t::ESP_LOG_ERROR, __func__,
			"failed to receive file");

		return Ret::Response;
	}

	// Trigger event notification
	Bft::OnFileBufferingFinished::notify(makeCustomDeallocationSharedFilePointer());

	// Restore the initial state, make it ready to accept new requests
	state.transferIntoMavlinkInitial();

	// Send a response
	handlePackResponse(aMavlinkMessage, aOnResponse, MAV_RESULT_ACCEPTED, esp_log_level_t::ESP_LOG_ERROR, __func__,
		"Initiating buffered file transfer");

	return Ret::Response;
}

std::shared_ptr<::Bft::File> BufferedFileTransfer::makeCustomDeallocationSharedFilePointer()
{
	return std::shared_ptr<::Bft::File>{&state.stageState.httpReceiving.file,
		[](::Bft::File *aBftFile)  // Deleter
		{
			aBftFile->close();
		}};
}

esp_err_t BufferedFileTransfer::onHttpFileDownloadChunk(const char *aChunk, std::size_t aChunkSize, void *aData)
{
	esp_err_t espErr = ESP_OK;

	if (aChunk == nullptr) {  // Check if this call is the announcement of the expected binary length (see `http` API). Knowing the expected length is crucial for allocating enough space
		if (aChunkSize > 0) {
			espErr = HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoHttpReceiving(aChunkSize);

			if (espErr != ESP_OK) {  // Cannot transfer into `Receiving` state. Most likely, unable to allocate memory
				ESP_LOGE(Mav::kDebugTag,
					"%s::%s: failed to transfer into receiving state, error=%d, stopping the process",
					kLogPreamble, __func__, static_cast<int>(espErr));
				HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoMavlinkInitial();
			}
		} else {  // Wrong arguments: file size=0
			ESP_LOGE(Mav::kDebugTag, "%s::%s: invalid announced file size=0", kLogPreamble, __func__);
			HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoMavlinkInitial();
			espErr = ESP_FAIL;
		}
	} else if (aChunkSize == 0 ) {  // Check if the transmission has been completed
		HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoMavlinkInitial();
		espErr = ESP_OK;
	} else {  // Regular transmission
		espErr = HttpDownloadContext::castFromVoid(aData).owner.state.onFileChunk(aChunk, aChunkSize);

		if (espErr != ESP_OK) {
			HttpDownloadContext::castFromVoid(aData).owner.state.transferIntoMavlinkInitial();
			ESP_LOGE(Mav::kDebugTag, "%s::%s Unable to handle file chunk. Stopping", kLogPreamble, __func__);
		}
	}

	return espErr;
}

inline bool BufferedFileTransfer::mavlinkMessageIsRepeatingMavCommandLong(const mavlink_message_t &aMavlinkMessage)
{
	// TODO XXX state / stage synchronization through mutex?
	return aMavlinkMessage.msgid == MAVLINK_MSG_ID_COMMAND_LONG
		&& mavlink_msg_command_long_get_target_component(&aMavlinkMessage) == Globals::getCompId()
		&& mavlink_msg_command_long_get_target_system(&aMavlinkMessage) == Globals::getSysId()
		&& mavlink_msg_command_long_get_command(&aMavlinkMessage) == kMavlinkCommandFetchFile
		&& state.stage != Stage::MavlinkInitial;  // The process is ongoing
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
	return static_cast<int>(lroundf(mavlink_msg_command_long_get_param1(&aMavlinkMessage)));
}

esp_err_t BufferedFileTransfer::State::transferIntoHttpReceiving(std::size_t aFileSize)
{
	esp_err_t result = ESP_OK;

	switch (stage) {  // Make sure the state is appropriate
		case Stage::HttpInitial: {
			stageState.httpReceiving.file = Bft::BufferedFileTransfer::getInstance().tryOpenFileWriteBinary(
				stageState.httpInitial.fileName.data(), aFileSize);

			if (stageState.httpReceiving.file.isValid()) {
				ESP_LOGE(Mav::kDebugTag, "%s::%s: failed to open file, name=\"%s\", size=%d", kLogPreamble, __func__,
					stageState.httpInitial.fileName.data(), aFileSize);
				result = ESP_FAIL;
			}

			break;
		}

		default:
			ESP_LOGE(Mav::kDebugTag, "%s::%s: cannot transfer into state", kLogPreamble, __func__);
			result = ESP_ERR_INVALID_STATE;

			break;
	}

	stage = Stage::HttpReceiving;

	return result;
}

esp_err_t BufferedFileTransfer::State::transferIntoHttpInitial(const char *aFileName)
{
	esp_err_t result = ESP_OK;

	switch (stage) {  // Make sure the state is appropriate
		case Stage::MavlinkInitial: {
			const std::size_t aFileNameLength = strlen(aFileName);

			if (aFileNameLength >= stageState.httpInitial.fileName.size()) {
				result = ESP_ERR_NO_MEM;
			}

			break;
		}

		default:
			result = ESP_ERR_INVALID_STATE;
			ESP_LOGE(Mav::kDebugTag, "%s::%s: cannot transfer into state", kLogPreamble, __func__);

			break;
	}

	stage = Stage::HttpInitial;

	return result;
}

void BufferedFileTransfer::State::transferIntoMavlinkInitial()
{
	switch (stage) {
		case Stage::HttpReceiving: {
			stageState.httpReceiving.file.close();

			break;
		}

		default:
			break;
	}

	stage = Stage::MavlinkInitial;
}

esp_err_t BufferedFileTransfer::State::onFileChunk(const char *aBuffer, std::size_t aBufferSize)
{
	esp_err_t result = ESP_OK;

	switch (stage) {  // Make sure the state is appropriate
		case Stage::HttpReceiving: {
			if (stageState.httpReceiving.file.isValid()) {
				// Try to write the chunk
				const auto nWritten = stageState.httpReceiving.file.append(
					reinterpret_cast<const std::uint8_t *>(aBuffer), aBufferSize);

				if (nWritten != aBufferSize) {
					ESP_LOGE(Mav::kDebugTag,
						"%s::%s: failed to write file chunk, chunk size=%d, actual written chunk length=%d",
						kLogPreamble, __func__, aBufferSize, nWritten);
				}
			} else {
				ESP_LOGE(Mav::kDebugTag, "%s::%s: cannot write into file, the file is invalid", kLogPreamble, __func__);
			}

			break;
		}

		default:
			result = ESP_ERR_INVALID_STATE;
			ESP_LOGE(Mav::kDebugTag, "%s::%s: cannot handle file chunk on this stage=Stage::HttpReceiving",
				kLogPreamble, __func__);

			break;
	}

	return result;
}

static bool tryEncodeAutopilotFileName(char *aBuffer, std::size_t aBufferSize, const mavlink_message_t &aMavlinkMessage)
{
	// Infer the name from the provided type
	const auto autopilotFileId = static_cast<int>(lround(mavlink_msg_command_long_get_param2(&aMavlinkMessage)));
	const char *autopilotFileName = AutopilotFile::tryGetNameById(autopilotFileId);

	if (autopilotFileName == nullptr) {
		ESP_LOGE(Mav::kDebugTag, "%s::%s: unable to infer autopilot file name from the provided file id=%d",
			kLogPreamble, __func__, autopilotFileId);

		return false;
	}

	// Try to fill the name into the resulting buffer
	const auto autopilotFileNameLength = strlen(autopilotFileName);

	if (autopilotFileNameLength >= aBufferSize) {
		ESP_LOGE(Mav::kDebugTag, "%s::%s: autopilot file name=\"%s\" won't fit into the buffer of size=%d",
			kLogPreamble, __func__, autopilotFileName, aBufferSize);

		return false;
	}

	std::fill_n(aBuffer, aBufferSize, '\0');
	strcpy(aBuffer, autopilotFileName);

	return true;
}

static inline void handlePackResponse(mavlink_message_t &aMavlinkMessage, Microservice::OnResponseSignature &aOnResponse,
	MAV_RESULT aMavResult, esp_log_level_t aEspLogLevel, const char *aContext, const char *aMessage)
{
	ESP_LOG_LEVEL(aEspLogLevel, Mav::kDebugTag, "%s::%s: %s", kLogPreamble, aContext, aMessage);
	auto commandAckFail = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMavlinkMessage, kMavlinkCommandFetchFile,
		MAV_RESULT_FAILED);
	commandAckFail.packInto(aMavlinkMessage);
	aOnResponse(aMavlinkMessage);
}

inline const char *AutopilotFile::tryGetNameById(int aAutopilotFileId)
{
	static constexpr std::array<const char *, AutopilotFile::N> kFileNames = {{
		"/dev/UavMonitor/show.bin",  // TODO: check that through running Pioneer SDK
	}};

	if (aAutopilotFileId >= kFileNames.size()) {
		ESP_LOGW(Mav::kDebugTag, "%s::%s: invalid file id=%d", kLogPreamble, __func__, aAutopilotFileId);
		return nullptr;
	} else {
		return kFileNames[aAutopilotFileId];
	}
}

}  // Mic
}  // Mav

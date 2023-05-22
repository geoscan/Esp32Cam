//
// fwupload.cpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Overriding local log level requires the following to be placed at the beginning of a file.
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_HTTP_DEBUG_LEVEL)
#include <esp_log.h>

#include "buffered_file_transfer/BufferedFileTransfer.hpp"
#include "http.h"
#include "http/utility.h"
#include "pages/pages.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

enum class InputBytesReceptionState {
	Receiving,
	Finished,
};

struct InputBytesIterationContext {
	union {
		struct {
			std::uint8_t *input;
			std::size_t inputSize;
		} receivingState;
		struct {
			esp_err_t espErr;
		} finishedState;
	};
	InputBytesReceptionState receptionState;

	explicit inline InputBytesIterationContext(std::uint8_t *aInput, std::size_t aContext)
	{
		receivingState = {aInput, aContext};
		receptionState = InputBytesReceptionState::Receiving;
	}

	explicit inline InputBytesIterationContext(esp_err_t aEspErr)
	{
		finishedState = {aEspErr};
		receptionState = InputBytesReceptionState::Finished;
	}
};

struct FileBufferingContext {
	union {
		void *stub;
		Bft::File file;
	};
	bool initialized;

	inline FileBufferingContext():
		stub{nullptr},
		initialized{false}
	{
	}
};

/// \returns ESP_OK, if successful. Sets `sErrorMessage`, and return error code
/// otherwise.
using InputBytesIterationHandler = esp_err_t(*)(const InputBytesIterationContext &aInputBytesIterationContext);

static constexpr std::size_t kIntermediateBufferMaxLength = 64;

/// \brief Checks relevant attributes of an incoming POST request
static esp_err_t httpdReqValidate(httpd_req_t *aHttpdReq);

/// \brief Receives input bytes. On each incoming chunk, notifies
/// `aInputBytesIterationHandler`
static esp_err_t httpdReqIterateReceiveInputBytes(httpd_req_t *aHttpdReq,
	InputBytesIterationHandler aInputBytesIterationHandler);

/// \brief Test implementation, proof-of-concept. Reads the input file
/// chunk-by-chunk, and produces debug output.
static esp_err_t handleInputBytesTest(const InputBytesIterationContext &aInputBytesIterationContext);

/// \brief Implements intermediate file buffering for its further transfer
static esp_err_t handleInputBytesFileBuffering(const InputBytesIterationContext &aInputBytesIterationContext);

static const char *sErrorMessage = nullptr;
FileBufferingContext sFileBufferingContext{};

static constexpr const char *debugPreamble()
{
	return "fwupload";
}

static esp_err_t httpdReqValidate(httpd_req_t *aHttpdReq)
{
	static constexpr const char *kFileExpectedName = "show";

	// Validate file name
	{
		char intermediateBuffer[kIntermediateBufferMaxLength] = {0};
		esp_err_t ret = httpdReqParameterValue(aHttpdReq, "file", intermediateBuffer, kIntermediateBufferMaxLength);

		if (ret != ESP_OK) {
			ESP_LOGE(httpDebugTag(), "%s: failed to get k/v pair", debugPreamble());

			return ret;
		}

		if (strcmp(kFileExpectedName, intermediateBuffer) != 0) {
			ESP_LOGE(httpDebugTag(), "%s: unsupported file name: \"%s\"", debugPreamble(), intermediateBuffer);

			return ESP_FAIL;
		}
	}

	// Validate file size
	{
		char intermediateBuffer[kIntermediateBufferMaxLength] = {0};
		esp_err_t ret = httpdReqParameterValue(aHttpdReq, "filesize", intermediateBuffer, kIntermediateBufferMaxLength);

		if (ret != ESP_OK) {
			ESP_LOGE(httpDebugTag(), "%s: missing argument \"filesize\"", debugPreamble());

			return ESP_FAIL;
		}
	}

	return ESP_OK;
}

static esp_err_t httpdReqIterateReceiveInputBytes(httpd_req_t *aHttpdReq,
	InputBytesIterationHandler aInputBytesIterationHandler)
{
	static constexpr bool shouldCloseConnection = false;
	static constexpr const char *kCallerLocation = "/fw";
	// Retrieve the pointer to scratch buffer for temporary storage
	char buffer[kIntermediateBufferMaxLength];
	std::size_t nReceived;

	// Content length of the request gives the size of the file being uploaded
	std::size_t remaining = aHttpdReq->content_len;

	while (remaining > 0) {
		ESP_LOGD(httpDebugTag(), "%s: Remaining size : %d", debugPreamble(), remaining);
		nReceived = httpd_req_recv(aHttpdReq, buffer, MIN(remaining, kIntermediateBufferMaxLength));

		// Receive the file part by part into a buffer
		if (nReceived <= 0) {
			if (nReceived == HTTPD_SOCK_ERR_TIMEOUT) {
				// Retry if timeout occurred
				continue;
			}

			ESP_LOGE(httpDebugTag(), "File reception failed!");
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(aHttpdReq, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
			aInputBytesIterationHandler(InputBytesIterationContext{ESP_FAIL});

			return ESP_FAIL;
		}

		aInputBytesIterationHandler(InputBytesIterationContext{reinterpret_cast<std::uint8_t *>(buffer), nReceived});

		/* Keep track of remaining size of
		 * the file left to be uploaded */
		remaining -= nReceived;
	}

	ESP_LOGI(httpDebugTag(), "File reception complete");
	aInputBytesIterationHandler(InputBytesIterationContext{ESP_OK});

	// Redirect to root to update the page
	httpd_resp_set_status(aHttpdReq, "303 See Other");
	httpd_resp_set_hdr(aHttpdReq, "Location", kCallerLocation);

	if (shouldCloseConnection) {
		httpd_resp_set_hdr(aHttpdReq, "Connection", "close");
	}

	httpd_resp_sendstr(aHttpdReq, "File uploaded successfully");

	return ESP_OK;
}

static esp_err_t handleInputBytesTest(const InputBytesIterationContext &aInputBytesIterationContext)
{
	(void)aInputBytesIterationContext;

	switch (aInputBytesIterationContext.receptionState) {
		case InputBytesReceptionState::Receiving:
			ESP_LOGI(httpDebugTag(), "%s: received %d bytes", debugPreamble(),
				aInputBytesIterationContext.receivingState.inputSize);

			break;

		default:
			break;
	}

	return ESP_OK;
}

static esp_err_t handleInputBytesFileBuffering(const InputBytesIterationContext &aInputBytesIterationContext)
{
	if (!Bft::BufferedFileTransfer::checkInstance()) {
		sErrorMessage = "`BufferedFileTransfer` is not initialized";

		return ESP_FAIL;
	}

	switch (aInputBytesIterationContext.receptionState) {
		case InputBytesReceptionState::Receiving:
			if (!sFileBufferingContext.initialized) {
				sFileBufferingContext.file = Bft::BufferedFileTransfer::getInstance().tryOpenFileWriteBinary(/*TODO: file name */"fw", /*TODO: file size*/ 64 * 1024);

				if (!sFileBufferingContext.file.isValid()) {
					sFileBufferingContext.file.close();
					sErrorMessage = "Unable to open file";

					return ESP_FAIL;
				}

				sFileBufferingContext.initialized = true;
			}

			sFileBufferingContext.file.append(aInputBytesIterationContext.receivingState.input,
				aInputBytesIterationContext.receivingState.inputSize);

			return ESP_OK;

		case InputBytesReceptionState::Finished:
			sFileBufferingContext.file.close();
			sFileBufferingContext.initialized = false;
			sFileBufferingContext.stub = nullptr;

			// TODO: notify subscribers

			return ESP_OK;
	}

	return ESP_OK;
}

extern "C" esp_err_t fwUploadPageHandler(httpd_req_t *aHttpdReq)
{
	ESP_LOGD(httpDebugTag(), "%s - running test page handler", debugPreamble());
	esp_err_t ret = ESP_OK;
	ret = httpdReqValidate(aHttpdReq);

	if (ESP_OK != ret) {
		ESP_LOGE(httpDebugTag(), "%s: failed to validate request", debugPreamble());

		return ret;
	}

	ret = httpdReqIterateReceiveInputBytes(aHttpdReq, handleInputBytesTest);

	if (ESP_OK != ret) {
		ESP_LOGE(httpDebugTag(), "%s: reception failed", debugPreamble());

		return ret;
	}

	return ESP_OK;
}

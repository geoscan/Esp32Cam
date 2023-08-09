//
// BufferedFileTransfer.hpp
//
// Created on: Aug 02, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//


#ifndef MAV_PRIV_INCLUDE_MICROSERVICE_BUFFEREDFILETRANSFER_HPP
#define MAV_PRIV_INCLUDE_MICROSERVICE_BUFFEREDFILETRANSFER_HPP

#include "Mavlink.hpp"
#include "Microservice.hpp"
#include "buffered_file_transfer/BufferedFileTransfer.hpp"
#include <esp_err.h>
#include <cstdint>

namespace Mav {
namespace Mic {

/// Waits for a command to fetch a file by URL. When it is issued:
/// 1. Fetches a file by URL over HTTP
/// 2. Pushes it back to the AP using FTP client protocol
class BufferedFileTransfer : public Mav::Microservice {
private:
	/// \brief Encapsulates the context passed along with
	/// `onHttpFileDownloadChunk`. See `http/client` API for more info.
	struct HttpDownloadContext {
	};

	enum class Stage {
		/// Waiting for a request
		Idle,

		/// In the process of fetching the binary file over HTTP
		ReceivingHttp,

		/// The file has been received successfully
		NotifyingSubscribers,
	};

	/// Encapsulates the buffered file transfer process at the Mav's side
	struct State {
		Bft::File bftFile;
		Stage stage;
	};

public:
	Ret process(mavlink_message_t &aMavlinkMessage, OnResponseSignature aOnResponse);

private:
	/// Process custom MAVLINK_COMMAND_LONG message
	///
	/// Fields (ignored, if not specified):
	/// - `target_system` = 1
	/// - `target_component` = MAV_COMP_ID_UDP_BRIDGE
	/// - `command` = MAV_CMD_USER_2
	/// - `confirmation` = <NUMBER OF THE ATTEMPT>
	/// - `param1` = <ID OF THE FILE TO FETCH>
	Ret onCommandLongFetchFile(mavlink_message_t &aMavlinkMessage, OnResponseSignature aOnResponse);

	/// Creates shared pointer over `BufferedFileTransfer::file` with a
	/// deallocator that automatically closes file.
	std::unique_ptr<Bft::File> makeCustomDeallocationSharedFilePointer();


	/// \brief Callback, a part of `http` client API. \sa "http/client/file.h"
	/// TODO implement
	static esp_err_t onHttpFileDownloadChunk(const char *aChunk, std::size_t aChunkSize, void *aData);

private:
	State state;
};

}  // Mic
}  // Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_BUFFEREDFILETRANSFER_HPP

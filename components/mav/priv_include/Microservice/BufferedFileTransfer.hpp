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
#include <array>
#include <cstdint>

namespace Mav {
namespace Mic {

/// \brief Waits for a command to fetch a file by URL. When it is issued:
/// 1. Fetches a file by URL over HTTP
/// 2. Pushes it back to the AP using FTP client protocol
///
/// \details This is a part of the buffered file transfer process. This
/// particular module serves the purpose of initial file fetching from a remote
/// file repository (namely: base station). \sa
/// `buffered_file_transfer/README.md`
class BufferedFileTransfer : public Mav::Microservice {
private:
	/// \brief Encapsulates the context passed along with
	/// `onHttpFileDownloadChunk`. See `http/client` API for more info.
	struct HttpDownloadContext {
		BufferedFileTransfer &owner;

		/// Boilerplate-reducing helper
		static inline HttpDownloadContext &castFromVoid(void *aHttpDownloadContext)
		{
			return *static_cast<HttpDownloadContext *>(aHttpDownloadContext);
		}
	};

	enum class Stage {
		/// Waiting for a MAVLink request
		MavlinkInitial = 0,

		/// Expecting HTTP file size
		HttpInitial,

		/// Got file size. Expecting chunks
		HttpReceiving,
	};

	/// Encapsulates the buffered file transfer process at the Mav's side,
	/// and also manages transitions in the context of BFT
	struct State {
		// TODO XXX state / stage synchronization through mutex?
		Stage stage;

		/// \pre Relies on the assumption that members stored here have
		/// non-RAII construction / destruction semantics
		union {
			struct MavlinkInitial {
			} mavlinkInitial;

			struct HttpInitial {
				std::array<char, 32> fileName;
			} httpInitial;

			struct {
				Bft::File file;
			} httpReceiving;
		} stageState;

		inline State():
			stage{Stage::MavlinkInitial},
			stageState{}
		{
		}

		/// If in `Idle` state, will try to allocate resources
		esp_err_t transferIntoHttpReceiving(std::size_t aFileSize);

		esp_err_t transferIntoHttpInitial(const char *aFileName);  // TODO: not invoked

		/// Deallocates all the resources, and falls back to the initial state.
		/// It is guaranteed to be able to do so from whatever state.
		void transferIntoMavlinkInitial();

		esp_err_t onFileChunk(const char *aBuffer, std::size_t aBufferSize);
	};

public:
	Ret process(mavlink_message_t &aMavlinkMessage, OnResponseSignature aOnResponse);
	BufferedFileTransfer() = default;
	~BufferedFileTransfer() override = default;

private:
	/// Process custom MAVLINK_COMMAND_LONG message
	///
	/// Fields (ignored, if not specified):
	/// - `target_system` = 1
	/// - `target_component` = MAV_COMP_ID_UDP_BRIDGE
	/// - `command` = MAV_CMD_USER_2
	/// - `confirmation` = <NUMBER OF THE ATTEMPT>
	/// - `param1` = <ID OF THE FILE TO FETCH>
	/// - `param2` = <AP FILE, 0.0 for Lua Show>
	Ret onCommandLongFetchFile(mavlink_message_t &aMavlinkMessage, OnResponseSignature aOnResponse);

	/// Creates shared pointer over `BufferedFileTransfer::file` with a
	/// deallocator that automatically closes file.
	std::shared_ptr<::Bft::File> makeCustomDeallocationSharedFilePointer();

	/// \brief Callback, a part of `http` client API. \sa "http/client/file.h"
	static esp_err_t onHttpFileDownloadChunk(const char *aChunk, std::size_t aChunkSize, void *aData);

private:
	State state;
};

}  // Mic
}  // Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_BUFFEREDFILETRANSFER_HPP

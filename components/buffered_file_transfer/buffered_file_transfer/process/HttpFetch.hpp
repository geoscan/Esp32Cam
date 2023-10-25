//
// HttpFetch.hpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCH_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCH_HPP

#include "buffered_file_transfer/BufferedFileTransfer.hpp"
#include <memory>

namespace Bft {

/// \brief Implements fetching of a file over HTTP, and its further transfer
/// using whatever scheme BFT is initialized for at a time. It may be used as
/// an entry point, as it triggers the entire process.
class HttpFetch {
public:
	/// \brief Fetches a file by HTTP URL, and performs buffered file transfer
	/// \param `aFileHttpUrl` the URL identifying the file
	/// \param `aBufferedFileTransferName` Target file identifier, semantically
	/// associated with the AP's file system structure. It may mismatch
	/// the file name used on the AP itself, and it gets translated into
	/// one under the hood. For example, \sa `SaluteFlashMemoryPartitionTable`
	void fetchFile(const char *aFileHttpUrl, const char *aBufferedFileTransferName);

private:
	/// \pre `bftFile` is valid
	void onFileChunkReceived(const char *aChunk, size_t aChunkSize);

	/// \brief Callback. The signature is compliant with HTTP file API (see
	/// `file.h`)
	static int onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData);

private:
	std::shared_ptr<Bft::File> bftFile;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCH_HPP

//
// TransferImplementor.hpp
//
// Created on: Sep 21, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_TRANSFERIMPLEMENTOR_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_TRANSFERIMPLEMENTOR_HPP

#include "buffered_file_transfer/storage/File.hpp"
#include <memory>

namespace Bft {

class TransferImplementor {
public:
	TransferImplementor();
	virtual ~TransferImplementor() = default;

	/// \brief Will notify each instance that a part of the RAM buffer should
	/// be flushed.
	/// \param `aIsLastChunk`, when true, signifies that there will be no more
	/// chunks, and the transmission session (if one is being held) should be
	/// gracefully ended.
	/// \param `aFile` is a `shared_ptr` encapsulation of the file with a
	/// custom deleter deallocating the resource through making the appropriate
	/// call to a `FileSystem` instance associated with it.
	virtual void onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk) = 0;

	/// \sa `onFileBufferingFinished`
	static void notifyAllOnFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk);
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_TRANSFERIMPLEMENTOR_HPP

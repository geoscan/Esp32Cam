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
	virtual ~TransferImplementor() = default;

	/// \brief Will notify each instance that a part of the RAM buffer should
	/// be flushed. DOES NOT necessarily mean the end of a transmission, unless
	/// `aIsLastChunk == true`
	/// \param `aIsLastChunk`, when true, signifies that there will be no more
	/// chunks pertaining to the transmitted file, and the transmission session
	/// (if one is being held) should be gracefully ended.
	/// \param `aFile` is a `shared_ptr` encapsulation of the file with a
	/// custom deleter deallocating the resource through making the appropriate
	/// call to a `FileSystem` instance associated with it.
	/// \post Implementor MUST NOT close the file. The `aFile`'s custom deleter
	/// MUST handle resource managing if required
	/// \pre When an instance is provided with `std::shared_ptr<File>`, the
	/// `File` is GUARANTEED to keep the last accumulated position
	/// \post The invoker MUST NOT hold any presuppositions on where the file's
	/// cursor will be after `TransferImplementor::onFileBufferingFinished()`
	/// is called. I.e. if one wants to preserve the position, it MUST NOT rely
	/// on the implementor, and handle it all by itself: it SHOULD cache the
	/// position before the call, and restore it later through calling
	/// `File::seek(...)`.
	virtual void onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk) = 0;

	/// \brief Notifies all instances
	/// \sa `onFileBufferingFinished`
	static void notifyAllOnFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk);
	static void subscribeInstanceForBufferingUpdates(TransferImplementor *aInstance);
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_TRANSFERIMPLEMENTOR_HPP

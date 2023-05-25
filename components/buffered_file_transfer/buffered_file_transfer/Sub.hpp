//
// Sub.hpp
//
// Created on: May 22, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SUB_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SUB_HPP

#include "buffered_file_transfer/storage/File.hpp"
#include "sub/Subscription.hpp"
#include <memory>
#include <vector>

namespace Bft {

/// `aBftFile` is supposed to store a deleter which performs the cleanup. It
/// implies that subscribers do not participate in file management.
using OnFileBufferingFinished = Rr::Util::Key<void(std::shared_ptr<::Bft::File> aBftfile), typename ::Sub::MutSyncTrait,
	std::list>;

struct TransferUpdateEvent {
	enum Status {
		StatusInProgress,
		StatusOk,
		StatusFailed,
	};

	Status status;

	union {
		struct {
			std::size_t currentOffset;
			std::size_t fileSize;
		} inProgressState;

		struct {
			const char *errorMessage;
		} failedState;
	};

	inline TransferUpdateEvent &setInProgress(std::size_t aCurrentOffset, std::size_t aFileSize)
	{
		status = StatusInProgress;
		inProgressState.currentOffset = aCurrentOffset;
		inProgressState.fileSize = aFileSize;

		return *this;
	}

	/// \pre `aErrorMessage` must have static lifetime
	inline TransferUpdateEvent &setFailed(const char *aErrorMessage)
	{
		status = StatusFailed;
		failedState.errorMessage = aErrorMessage;

		return *this;
	}
};

using OnTransferUpdate = Rr::Util::Key<void(TransferUpdateEvent), typename ::Sub::MutSyncTrait, std::list>;

}  // namespace Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SUB_HPP

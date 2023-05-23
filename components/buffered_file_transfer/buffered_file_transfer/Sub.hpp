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

}  // namespace Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SUB_HPP

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

namespace Bft {

using OnFileBufferingFinished = typename Rr::Util::Key<void(std::shared_ptr<::Bft::File>), ::Sub::MutSyncTrait, std::vector, void>;

}  // namespace Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SUB_HPP

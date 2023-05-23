//
// buffered_file_transfer.hpp
//
// Created: 2023-05-18
//  Author:
//

#if !defined(COMPONENTS_BUFFERED_FILE_TRANSFER_HPP_)
#define COMPONENTS_BUFFERED_FILE_TRANSFER_HPP_

namespace Bft {

void init();

static constexpr const char *kDebugTag = "[buffered_file_transfer]";

static constexpr const char *debugTag()
{
	return kDebugTag;
}

}  // namespace Bft

#endif  // COMPONENTS_BUFFERED_FILE_TRANSFER_HPP_

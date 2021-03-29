//
// Buffer.hpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_UTILITY_UTILITY_BUFFER_HPP
#define COMPONENTS_UTILITY_UTILITY_BUFFER_HPP

#include <utility>

namespace Utility {

class Buffer {
	void *mData;
	std::size_t mSize;
public:
	Buffer(void *data = nullptr, std::size_t size = 0);
	Buffer(Buffer &&);
	Buffer &operator=(Buffer &&);

	Buffer(const Buffer &) = delete;
	Buffer &operator=(const Buffer &) = delete;

	void *data();
	std::size_t size();
	virtual ~Buffer() = default;
};

}  // namespace Utility

#endif  // BUFFER_HPP

//
// Buffer.cpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/Buffer.hpp"

using namespace Utility;

Buffer::Buffer(void *data, std::size_t size) : mData(data), mSize(size)
{
}

Buffer::Buffer(Buffer &&buffer) : mData(buffer.mData), mSize(buffer.mSize)
{
	buffer.mData = nullptr;
	buffer.mSize = 0;
}

Buffer &Buffer::operator=(Buffer &&buffer)
{
	mData = buffer.mData;
	mSize = buffer.mSize;
	buffer.mData = nullptr;
	buffer.mSize = 0;
	return *this;
}

void *Buffer::data()
{
	return mData;
}

std::size_t Buffer::size()
{
	return mSize;
}


//
// Types.hpp
//
// Created on:  Aug. 20, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//


// --------- Types used across multiple project's parts --------- //


#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstddef>

namespace Rtsp {

// Const data buffer
struct Data {
	void  *data      {nullptr};
	const size_t len {0};
};


// -------------------- Parser-related types -------------------- //

template <typename T>
struct Optval {
	T    val;
	bool isInit {false}; // true, if initialized
}

// Represents common request fields
struct Request {
	enum {
		NotStated;
	} RequestType {NotStated};
	Optional<unsigned> cseq;
	Optional<unsigned> session;
};

} // Rtsp

#endif // TYPES_HPP

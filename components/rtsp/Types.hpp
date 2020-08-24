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
private:
	T    value;
	bool isInit {false}; // true, if initialized
public:
	T &operator=(const T &v) {
		value = v;
		isInit = true;
		return value;
	}
	T &operator=(T &&v) {
		value = v;
		isInit = true;
		return value;
	}
	bool isVal() const {
		return isInit;
	}
	const T &val() const {
		return value;
	}
};

// Represents common request fields
struct Request {
	enum {
		NotStated
	} requestType {NotStated};
	Optval<unsigned> cseq;
	Optval<unsigned> session;
	Optval<unsigned> clientPort;
};

} // Rtsp

#endif // TYPES_HPP

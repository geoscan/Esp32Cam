//
// Types.hpp
//
// Created on:  Aug. 20, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//


// -------- Types used across multiple project's classes -------- //


#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstddef>

namespace Rtsp {

// Const data buffer
struct Data {
	const void   *data {nullptr};
	const size_t len   {0};
};


// -------- Parsed user requests -------- //


struct Describe {
};

struct Request {
	enum {
		Describe,
		Setup,
		Teardown,
		Play,
		Pause
	} const reqType;
	union {
		const Describe describe;
	};
};

} // Rtsp

#endif // TYPES_HPP

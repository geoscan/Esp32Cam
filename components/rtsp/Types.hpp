//
// Types.hpp
//
// Created on:  Aug. 20, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_TYPES_HPP
#define COMPONENTS_RTSP_TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <memory>



// --------- Types used across multiple project's parts --------- //

namespace Rtsp {

// Const data buffer view
struct Data {
	const void *data;
	size_t len;
};

using SessionId = unsigned;

enum class StatusCode {
	Ok = 200,
	MethodNotAllowed = 405,
	SessionNotFound = 454,
	MethodNotValidInThisState = 455,
	UnsupportedTransport = 461
};

// -------------------- Parser-related types -------------------- //


template <typename T>
struct Optval final {
private:
	T    value;
	bool isInit {false}; // true, if initialized
public:
	Optval()
	{
	}
	Optval(const Optval &o) : value(o.value), isInit(o.isInit)
	{
	}
	Optval(Optval &&o) : value(std::move(o.value)), isInit(o.isInit)
	{
		o.isInit = false;
	}
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
	operator bool() const
	{
		return isVal();
	}
	const T &operator*() const
	{
		return val();
	}
};


enum class RequestType {
	NotStated,
	Describe,
	Setup,
	Teardown,
	Play,
	Pause
};


// Represents common request fields
struct Request final {
	Optval<RequestType> requestType;
	Optval<unsigned>    cseq;
	Optval<unsigned>    session;
	Optval<unsigned>    clientPort;
	Optval<bool>        udp; // Transport: client requests using UDP
};

} // Rtsp

#endif // COMPONENTS_RTSP_TYPES_HPP

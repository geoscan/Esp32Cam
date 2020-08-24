//
// Types.hpp
//
// Created on:  Aug. 20, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_TYPES_HPP
#define COMPONENTS_RTSP_TYPES_HPP



// --------- Types used across multiple project's parts --------- //



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

enum class RequestType {
	Describe,
	Setup,
	Teardown,
	Play,
	Pause,
	NotStated
};

// Represents common request fields
struct Request {
	Optval<RequestType> requestType;
	Optval<unsigned>    cseq;
	Optval<unsigned>    session;
	Optval<unsigned>    clientPort;
	Optval<bool>        udp; // Transport: client requests using UDP
};

} // Rtsp

#endif // COMPONENTS_RTSP_TYPES_HPP

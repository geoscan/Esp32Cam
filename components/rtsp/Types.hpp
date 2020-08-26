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
#include <type_traits>

#include "parser_debug_flag.hpp"

#if PARSER_DEBUG != 1
#include "asio.hpp"
#endif

namespace Rtsp {

// ------------------------ Identifiable ------------------------ //

template <typename T>
class Identifiable {
	static unsigned boundId;
	#if PARSER_DEBUG != 1
	static asio::detail::mutex mutex;
	#endif
	unsigned mId;

	Identifiable(unsigned cid)
	{
		mId = cid;
	}
public:
	static constexpr unsigned NoId = 0;
	unsigned id() const
	{
		return this->mId;
	}
	static std::unique_ptr<T> key(unsigned cid)
	{
		Identifiable<T> *dummy = new Identifiable<T>(cid);
		return std::unique_ptr<T>(reinterpret_cast<T *>(dummy));
	}
	Identifiable() /*: mId(boundId++)*/
	{
		#if PARSER_DEBUG != 1
		this->mutex.lock();
		#endif

		mId = boundId++;

		#if PARSER_DEBUG != 1
		this->mutex.unlock();
		#endif
	}
	Identifiable(const Identifiable &) : Identifiable()
	{
	}
	Identifiable(Identifiable &&i)
	{
	}
	Identifiable &operator=(const Identifiable &)
	{
	}
	Identifiable &operator=(Identifiable &&i)
	{
		mId = i.mId;
	}
	virtual ~Identifiable()
	{
	}

	struct Less {
	private:
		using Type = Identifiable<T>;
		unsigned val(const Type &v) const
		{
			return v.id();
		}
		unsigned val(unsigned v) const
		{
			return v;
		}
		template <typename ... Args>
		unsigned val(const std::unique_ptr<Args...> &v) const
		{
			return v->id();
		}
		template <typename ... Args>
		unsigned val(const std::shared_ptr<Args...> &v) const
		{
			return v->id();
		}
	public:
		template <typename Ltype, typename Rtype>
		bool operator()(const Ltype &l, const Rtype &r) const
		{
			return val(l) < val(r);
		}
	};
};

template <typename T>
unsigned Identifiable<T>::boundId = Identifiable<T>::NoId + 1;


// ------------------------- Lock guard ------------------------- //

#if PARSER_DEBUG != 1

/// Poor man's lock guard. More on this topic:
/// https://en.cppreference.com/w/cpp/thread/lock_guard
struct LockGuard {
private:
	asio::detail::mutex &mut;
public:
	LockGuard(asio::detail::mutex &mutex) : mut(mutex)
	{
		mutex.lock();
	}
	~LockGuard()
	{
		mut.unlock();
	}
};
#endif // PARSER_DEBUG != 1


// ---------------------- Data buffer view ---------------------- //

struct Data {
	const void *data;
	size_t len;
};


// -------------------- Parser-related types -------------------- //

using SessionId = unsigned;

enum class StatusCode {
	Ok = 200,
	MethodNotAllowed = 405,
	SessionNotFound = 454,
	MethodNotValidInThisState = 455,
	UnsupportedTransport = 461
};


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


enum class Format {
	NotStated, // unsupported
	Mjpeg
};


// Represents common request fields
struct Request final {
	Optval<RequestType> requestType;
	Optval<unsigned>    cseq;
	Optval<unsigned>    session;
	Optval<unsigned>    clientPort;
	Optval<bool>        udp; // Transport: client requests using UDP
	Optval<Format>      format;
};

} // Rtsp

#endif // COMPONENTS_RTSP_TYPES_HPP

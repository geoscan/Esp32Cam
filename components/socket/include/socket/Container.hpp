//
// Container.hpp
//
// Created on: Feb 10, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SOCKET_PRIV_INCLUDE_CONTAINER_HPP
#define SOCKET_PRIV_INCLUDE_CONTAINER_HPP

#include <tuple>
#include <list>
#include <asio.hpp>
#include <cassert>
#include <utility>

namespace Sock {

namespace ContainerImpl {

struct CallRemoteEndpoint {  ///< SFINAE wrapper
	template <class T, class ...Ta>
	static auto call(T &&aT, Ta &&...aArgs) -> decltype(aT.remote_endpoint(std::forward<Ta>(aArgs)...))
	{
		return aT.remote_endpoint(std::forward<Ta>(aArgs)...);
	}

	static void call(...)  ///< SFINAE fallback
	{
		assert(false);
	}
};

template <class Proto>
struct GetSocketType;

template<>
struct GetSocketType<asio::ip::udp> {
	using Type = typename asio::ip::udp::socket;
};

template <>
struct GetSocketType<asio::ip::tcp> {
	using Type = typename asio::ip::tcp::socket;
};

template <class T>
using ContainerBase = typename std::list<T>;

template <class T>
struct Container;

template <class Tproto, template <class...> class Tobject>
struct Container<Tobject<Tproto>> : public ContainerBase<Tobject<Tproto>> {

	using Id = typename std::tuple<asio::ip::basic_endpoint<Tproto>, std::uint16_t /*local port*/>;
	using Socket = typename GetSocketType<Tproto>::Type;
	using Base = ContainerBase<Tobject<Tproto>>;
	using Value = typename Base::value_type;
	using Iterator = typename Base::iterator;

	using Base::Base;
	using Base::emplace;
	using Base::emplace_back;
	using Base::end;
	using Base::begin;
	using Base::push_back;
	using Base::pop_back;
	using Base::back;
	using Base::erase;

	typename Base::iterator find(std::uint16_t aLocalPort);
	typename Base::iterator find(const asio::ip::basic_endpoint<Tproto> &aRemoteEndpoint, std::uint16_t aLocalPort);
	typename Base::iterator find(const asio::ip::basic_endpoint<Tproto> &aRemoteEndpoint);

	template <class ...Ts>
	void pop(Ts &&...aTs);
};

}  // namespace ContainerImpl

template <class Tproto>
using Container = typename ContainerImpl::Container<Tproto>;

}  // namespace Sock

#include "Container.impl"

#endif // SOCKET_PRIV_INCLUDE_CONTAINER_HPP

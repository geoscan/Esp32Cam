//
// IntsanceRegistry.hpp
//
// Created on: Apr 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_COMM_INSTANCEREGISTRY_HPP_
#define UTILITY_UTILITY_COMM_INSTANCEREGISTRY_HPP_

#include <algorithm>
#include <mutex>

namespace Utility {
namespace Comm {

template <class T, template <class ...> class Tcontainer>
struct InstanceRegistry {
	Tcontainer<T *> instances;
	void add(T &aInst)
	{
		auto it = std::find(std::begin(instances), std::end(instances), &aInst);

		if (std::end(instances) == it) {
			*std::inserter(instances, it) = &aInst;
		}
	}
	void remove(T &aInst)
	{
		auto it = std::find(std::begin(instances), std::end(instances), &aInst);

		if (std::end(instances) != it) {
			instances.erase(it);
		}
	}
};

template <class T, template <class ...> class Tc>
struct SyncedStorage {
private:
	std::mutex mutex;
	Tc<T> storage;
public:
	template <class ...Ts>
	SyncedStorage(Ts &&...aArgs) : mutex{}, storage{std::forward<Ts>(aArgs)...}
	{
	}

	template <class ...Ts>
	T &emplace(Ts &&...aArgs)
	{
		std::lock_guard<std::mutex> lock{mutex};
		(void)lock;

		T t{std::forward<Ts>(aArgs)...};

		auto it = std::find(std::begin(storage), std::end(storage), t);

		if (std::end(storage) != it) {
			return (*std::inserter(storage, it) = std::move(t));
		} else {
			return *it;
		}
	}
};

}  // namespace Comm
}  // namespace Utility

#endif // UTILITY_UTILITY_COMM_INSTANCEREGISTRY_HPP_

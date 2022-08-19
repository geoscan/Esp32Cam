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
#include <vector>

namespace Ut {
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

template <class T>
struct OrderedInstanceRegistry : std::vector<T *> {
public:
	using std::vector<T *>::insert;
	using std::vector<T *>::clear;

	OrderedInstanceRegistry(std::size_t aSizeHint) : std::vector<T *>(aSizeHint)
	{
		clear();
	}

	/// \brief Order-preserving modifier
	///
	void add(T &aInstance)
	{
		if (std::find(std::begin(*this), std::end(*this), &aInstance) == std::end(*this)) {
			insert(std::lower_bound(std::begin(*this), std::end(*this), aInstance,
				[](const T *aStored, const T &aCandidate) {return *aStored < aCandidate; }), &aInstance);
		}
	}

	void remove(T &aInstance)
	{
		this->erase(std::find(std::begin(*this), std::end(*this), &aInstance));
	}

	/// \brief Iterates over a range of instances that are equal to the key.
	///
	/// \details The target instance is searched w/ binary search, so the operation is time efficient.
	///
	/// \pre operator==(const C &, const K &) and operator<(const C &, const K &) must be defined.
	///
	template <class C, class K>
	void foreach(C &&aCallable, const K &aKey)
	{
		auto it = std::lower_bound(std::begin(*this), std::end(*this), aKey,
			[](const T *aStored, const K &aCandidate) {return *aStored < aCandidate; });

		while (it != std::end(*this)) {
			if (*it == aKey) {
				aCallable(it);
				++it;
			} else {
				break;
			}
		}
	}
};

/// \brief Relies on presence of `push_back` in Tc
///
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
		storage.push_back(T{std::forward<Ts>(aArgs)...});

		return storage.back();
	}
};

}  // namespace Comm
}  // namespace Ut

#endif // UTILITY_UTILITY_COMM_INSTANCEREGISTRY_HPP_

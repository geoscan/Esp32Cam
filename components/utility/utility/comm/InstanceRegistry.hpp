//
// IntsanceRegistry.hpp
//
// Created on: Apr 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_COMM_INSTANCEREGISTRY_HPP_
#define UTILITY_UTILITY_COMM_INSTANCEREGISTRY_HPP_

#include <algorithm>

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

}  // namespace Comm
}  // namespace Utility

#endif // UTILITY_UTILITY_COMM_INSTANCEREGISTRY_HPP_

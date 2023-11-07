//
// MakeSingleton.hpp
//
// Created on: Feb 10, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_MAKESINGLETON_HPP
#define UTILITY_UTILITY_MAKESINGLETON_HPP

#include <cassert>

namespace Ut {

/// \brief Implements the "Singleton" architectural approach
/// \tparam `kStaticInstanceUniqueIdentifier` is as compile-time marker that is
/// used to differentiate between various instances of the same kind.
template <class T, int kStaticInstanceUniqueIdentifier = 0>
class MakeSingleton {
private:
	static T *instance;

protected:
	MakeSingleton(const MakeSingleton &) = delete;
	MakeSingleton(MakeSingleton &&) = delete;
	MakeSingleton &operator=(const MakeSingleton &) = delete;
	MakeSingleton &operator=(MakeSingleton &&) = delete;

	MakeSingleton(T &aInstance)
	{
		setInstance(aInstance);
	}

	MakeSingleton()
	{
		assert(!checkInstance());
		MakeSingleton<T>::setInstance(*static_cast<T *>(this));
	}

public:
	static T &getInstance()
	{
		assert(instance != nullptr);
		return *instance;
	}

	static void setInstance(T &aInstance)
	{
		assert(instance == nullptr);
		instance = &aInstance;
	}

	static bool checkInstance()
	{
		return instance != nullptr;
	}
};

template <class T, int kStaticInstanceUniqueIdentifier>
T *MakeSingleton<T, kStaticInstanceUniqueIdentifier>::instance = nullptr;

}  // namespace Ut

#endif // UTILITY_UTILITY_MAKESINGLETON_HPP

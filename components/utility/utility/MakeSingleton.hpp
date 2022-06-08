//
// MakeSingleton.hpp
//
// Created on: Feb 10, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_MAKESINGLETON_HPP
#define UTILITY_UTILITY_MAKESINGLETON_HPP

#include <cassert>

namespace Utility {

template <class T>
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

template <class T>
T *MakeSingleton<T>::instance = nullptr;

}  // namespace Utility

#endif // UTILITY_UTILITY_MAKESINGLETON_HPP

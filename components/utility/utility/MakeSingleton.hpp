//
// MakeSingleton.hpp
//
// Created on: Feb 10, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_MAKESINGLETON_HPP
#define UTILITY_UTILITY_MAKESINGLETON_HPP

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

	MakeSingleton() = delete;
	MakeSingleton(T &);

public:
	static T &getInstance();
	static void setInstance(T &);
	static bool checkInstance();
};

}  // namespace Utility

#include "MakeSingleton.impl"

#endif // UTILITY_UTILITY_MAKESINGLETON_HPP

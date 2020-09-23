//
// Run.hpp
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_UTILITY_RUN_HPP
#define COMPONENTS_UTILITY_RUN_HPP

namespace Utility {

//
// Convenient wrapper which may be seamlessly passed to pthread_create(4)
//
// Runnable -- class with public method run(0)
// instance -- instance of Runnable
//

template <typename Runnable>
void *run(void *instance)
{
	Runnable *runnable = reinterpret_cast<Runnable *>(instance);
	runnable->run();
	return nullptr;
}

}  // namespace Utility

#endif  // COMPONENTS_UTILITY_RUN_HPP

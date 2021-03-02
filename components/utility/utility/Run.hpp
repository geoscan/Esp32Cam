//
// Run.hpp
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_UTILITY_RUN_HPP
#define COMPONENTS_UTILITY_RUN_HPP

#include <pthread.h>
#include <esp_pthread.h>

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

void setThreadCoreAffinity(int coreAffinity);

template <typename Runnable>
pthread_t threadRun(Runnable &instance, int coreAffinity = 0)
{
	setThreadCoreAffinity(coreAffinity);

	pthread_t threadDescriptor;
	pthread_create(&threadDescriptor, 0, run<Runnable>, &instance);
	return threadDescriptor;
}

template <typename Runnable>
pthread_t threadRun(Runnable &instance, const pthread_attr_t &attr, int coreAffinity = 0)
{
	setThreadCoreAffinity(coreAffinity);

	pthread_t threadDescriptor;
	pthread_create(&threadDescriptor, &attr, run<Runnable>, &instance);
	return threadDescriptor;
}

}  // namespace Utility

#endif  // COMPONENTS_UTILITY_RUN_HPP

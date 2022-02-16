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

namespace Threading {

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

///
/// \brief Config is a convenience RAII-wrapper over standard C-style esp
/// config setters. It initiates a config structure on construction, and
/// applies it when destructed.
///
class Config {
	esp_pthread_cfg_t config;

public:
	///
	/// \param aFromDefault Initialize struct from default config. If false, a
	/// currently used config will serve as a template
	///
	Config(bool aFromDefault);
	~Config();

	// setters
	Config &core(int a = 0);  ///< Pin to core
	Config &stack(int a = CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT);  ///< stack size
	Config &inherit(bool inheritCfg = true);  ///< use this config further
	Config &name(const char *a);  ///< name of the thread
	Config &priority(int a = CONFIG_PTHREAD_TASK_PRIO_DEFAULT);
};

template <typename Runnable>
pthread_t threadRun(Runnable &instance, int coreAffinity = 0)
{
	pthread_t threadDescriptor;
	pthread_create(&threadDescriptor, 0, run<Runnable>, &instance);
	return threadDescriptor;
}

template <typename Runnable>
pthread_t threadRun(Runnable &instance, const pthread_attr_t &attr, int coreAffinity = 0)
{
	pthread_t threadDescriptor;
	pthread_create(&threadDescriptor, &attr, run<Runnable>, &instance);
	return threadDescriptor;
}

}  // namespace Threading

}  // namespace Utility

#endif  // COMPONENTS_UTILITY_RUN_HPP

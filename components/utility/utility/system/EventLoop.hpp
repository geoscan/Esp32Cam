//
// EventLoop.hpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_EVENTLOOP_HPP_
#define UTILITY_UTILITY_EVENTLOOP_HPP_

#include "utility/comm/InstanceRegistry.hpp"
#include <esp_event.h>
#include <string>
#include <list>

namespace Ut {
namespace Sys {

class Handler;

///
/// \brief Wrapper over esp event loop
///
class Loop : public esp_event_loop_args_t {
private:
public:
	Loop(std::size_t aQueueSize, const char *aName, unsigned aTaskPrio, std::size_t aStackSize, unsigned aCoreId,
		bool aStart = true);
	~Loop();

	esp_err_t run(TickType_t aTicks);  ///< Sync run
	const std::string &getName() const;
	friend esp_err_t reg(const char *aName, Handler &, esp_event_base_t aEventBase, int32_t aEventId);
	friend esp_err_t unreg(const char *aName, Handler &, esp_event_base_t aEventBase, int32_t aEventId);

	template <class T>
	friend esp_err_t post(const char *aName, T&&, esp_event_base_t aEventBase, std::int32_t aEventId, TickType_t aTicks);

	template <class T>
	friend esp_err_t post(const char *aName, T*, esp_event_base_t aEventBase, std::int32_t aEventId, TickType_t aTicks);

private:
	static void eventStaticCallback(void *aHandlerArg, esp_event_base_t aEventBase, std::int32_t aEventId,
		void *aEventData);

private:
	std::string name;
	esp_event_loop_handle_t espEventLoopHandle;

	static struct InstanceRegistry : Ut::Comm::InstanceRegistry<Loop, std::list> {
		Loop *find(const char *);
	} instanceRegistry;
};

class Handler {
public:
	void onEspEvent(esp_event_base_t aEventBase, std::int32_t aEventId, void *aEventData);

	friend esp_err_t reg(const char *aName, Handler &, esp_event_base_t aEventBase, int32_t aEventId);
	friend esp_err_t unreg(const char *aName, Handler &, esp_event_base_t aEventBase, int32_t aEventId);

private:
	esp_event_handler_instance_t instance;
};

esp_err_t reg(const char *aName, Handler &, esp_event_base_t aEventBase, int32_t aEventId);
esp_err_t unreg(const char *aName, Handler &, esp_event_base_t aEventBase, int32_t aEventId);

template <class T>
esp_err_t post(const char *aName, T&&, esp_event_base_t aEventBase, std::int32_t aEventId, TickType_t aTicks);

template <class T>
esp_err_t post(const char *aName, T*, esp_event_base_t aEventBase, std::int32_t aEventId, TickType_t aTicks);

}  // namespace Sys
}  // namespace Ut

#include "EventLoop.impl"

#endif // UTILITY_UTILITY_EVENTLOOP_HPP_

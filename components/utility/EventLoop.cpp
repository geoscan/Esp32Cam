//
// EventLoop.cpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/EventLoop.hpp"
#include <cstring>

namespace Ut {
namespace Ev {

esp_err_t reg(const char *aName, Handler &aHandler, esp_event_base_t aEventBase, int32_t aEventId)
{
	auto *loopInstance = Loop::instanceRegistry.find(aName);
	esp_err_t ret = ESP_FAIL;

	if (nullptr != loopInstance) {
		ret = esp_event_handler_instance_register_with(loopInstance->espEventLoopHandle, aEventBase, aEventId,
			Loop::eventStaticCallback, &aHandler, &aHandler.instance);
	}

	return ret;
}

esp_err_t unreg(const char *aName, Handler &aHandler, esp_event_base_t aEventBase, int32_t aEventId)
{
	auto *loopInstance = Loop::instanceRegistry.find(aName);
	esp_err_t ret = ESP_FAIL;

	if (nullptr != loopInstance) {
		ret = esp_event_handler_instance_unregister_with(loopInstance->espEventLoopHandle, aEventBase, aEventId,
			aHandler.instance);
	}

	return ret;
}

Loop::Loop(std::size_t aQueueSize, const char *aName, unsigned aTaskPrio, std::size_t aStackSize, unsigned aCoreId,
	bool aStart) :
	name{aName}
{
	esp_event_loop_args_t args;
	args.queue_size = aQueueSize;
	args.task_core_id = aCoreId;
	args.task_priority = aTaskPrio;

	if (aStart) {
		args.task_name = name.c_str();
	}

	esp_event_loop_create(&args, &espEventLoopHandle);
}

Loop::~Loop()
{
	esp_event_loop_delete(espEventLoopHandle);
}

esp_err_t Loop::run(TickType_t aTicks)
{
	return esp_event_loop_run(espEventLoopHandle, aTicks);
}

void Loop::eventStaticCallback(void *aHandlerArg, esp_event_base_t aEventBase, int32_t aEventId, void *aEventData)
{
	static_cast<Handler *>(aHandlerArg)->onEspEvent(aEventBase, aEventId, aEventData);
}

Loop *Loop::InstanceRegistry::find(const char *aName)
{
	Loop *ret = nullptr;
	auto it = std::find_if(std::begin(instanceRegistry.instances), std::end(instanceRegistry.instances),
		[aName](Loop *aInst) { return 0 == std::strcmp(aName, aInst->getName().c_str()); });

	if (std::end(instanceRegistry.instances) != it) {
		ret = *it;
	}

	return ret;
}

}  // namespace Ev
}  // namespace Ut

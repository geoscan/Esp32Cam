//
// Thread.cpp
//
// Created on: Oct 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <embmosse/Port/Task.hpp>
#include "utility/thr/Threading.hpp"
#include "tracking/tracking.hpp"
#include "Thread.hpp"
#include <esp_log.h>

/// \brief Switches sequentially between core 0 and core 1 to achieve physical parallellism
///
static int corePin()
{
	static int sCoreAffinity = 0;
	static constexpr int knCores = 2;
	int ret = sCoreAffinity;
	sCoreAffinity = (sCoreAffinity + 1) % knCores;

	return ret;
}

std::unique_ptr<Mosse::Port::Thread> Trk::Thread::makeFromTask(Mosse::Port::Task &aTask)
{
	return Mosse::Port::Thread::makeUnique<Thread>(aTask);
}

void stub(void *)
{
	while (true);
}

void Trk::Thread::start()
{
	ESP_LOGI(Trk::kDebugTag, "Creating tracking thread");
	auto res = xTaskCreatePinnedToCore(threadTask, NULL, 1024 * 3, this, Ut::Thr::FreertosTask::PriorityMedium,
		&taskHandle, corePin());

	if (res != pdPASS) {
		ESP_LOGE(Trk::kDebugTag, "Failed to create a tracking thread");
	} else {
		ESP_LOGI(Trk::kDebugTag, "Successfully created tracking thread");
	}
}

void Trk::Thread::threadTask(void *aInstance)
{
	Trk::Thread &instance = *reinterpret_cast<Trk::Thread *>(aInstance);
	vTaskDelay(1);
	ESP_LOGI(Trk::kDebugTag, "started tracking thread");
	instance.task()->run();
	ESP_LOGI(Trk::kDebugTag, "finished tracking thread");
	while (true);
}

//
// camera_thread.cpp
//
// Created on: Apr 07, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "CameraStream.hpp"
#include "camera_thread/camera_thread.hpp"
#include "utility/thr/Threading.hpp"
#include <sdkconfig.h>

using namespace CameraThread;

namespace CameraThread {

const char *kDebugTag = "[camera_thread]";

}  // namespace CameraThread

static void cameraThreadTask(void *)
{
	static CameraStream cameraStream;
	cameraStream();
}

void cameraThreadInit()
{
#if !CONFIG_DRIVER_OV2640_USE_HOOKS
	ESP_LOGI(kDebugTag, "Initializing");
	static TaskHandle_t taskHandle;
	constexpr std::size_t kStackSize = 1024 * 3;
	constexpr void *kArg = nullptr;
	constexpr UBaseType_t kPriority = Ut::Thr::FreertosTask::PriorityLowest + 1;
	xTaskCreatePinnedToCore(cameraThreadTask, "camera_thread", kStackSize, kArg, kPriority, &taskHandle,
		CONFIG_CAMERA_THREAD_PIN_TO_CORE);
#endif
}

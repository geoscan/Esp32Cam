//
// socket.cpp
//
// Created on: Feb 10, 2022
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#include <sdkconfig.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_SOCKET_DEBUG_LEVEL)

#include "socket/socket.hpp"
#include "socket/Api.hpp"
#include "Task.hpp"
#include <thread>
#include <chrono>
#include "utility/thr/Threading.hpp"
#include <esp_log.h>

namespace Sock {

static void apiInit(Sock::Api &aApi)
{
	constexpr auto kUdpOpen = {8001 /* AP protocol (MAVLink) */};
	asio::error_code err;

	for (auto port : kUdpOpen) {
		ESP_LOGI(Sock::kDebugTag, "Sock::apiInit() opening udp socket on port %d", port);
		aApi.openUdp(port, err);
	}
}

void start(asio::io_context &aIoContext)
{
	static std::mutex syncAsyncMutex;
	static Task task{aIoContext};
	static Sock::Api api{aIoContext, syncAsyncMutex};

	apiInit(api);

	Utility::Thr::Config(true).core(0).stack(CONFIG_ESP32_PTHREAD_TASK_STACK_SIZE_DEFAULT + 1024);
	static std::thread thread{&Task::run, &task};
	(void)api;
	(void)thread;
}

void start()
{
	esp_log_level_set(Sock::kDebugTag, (esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL);
	ESP_LOGD(Sock::kDebugTag, "Debug log test");
	ESP_LOGV(Sock::kDebugTag, "Verbose log test");
	static asio::io_context ioContext;
	Sock::start(ioContext);
}

}  // namespace Sock

#include "utility/thr/Threading.hpp"

void Utility::Thr::setThreadCoreAffinity(int coreAffinity)
{
	auto cfg = esp_pthread_get_default_config();
	cfg.pin_to_core = coreAffinity;
	esp_pthread_set_cfg(&cfg);
}

Utility::Thr::Config::Config(bool aFromDefault): config{}
{
	if (!aFromDefault) {
		esp_pthread_get_cfg(&config);
	} else {
		config = esp_pthread_get_default_config();
	}
}

Utility::Thr::Config::~Config()
{
	esp_pthread_set_cfg(&config);
}

Utility::Thr::Config &Utility::Thr::Config::core(int a)
{
	config.pin_to_core = a;
	return *this;
}

Utility::Thr::Config &Utility::Thr::Config::inherit(bool a)
{
	config.inherit_cfg = a;
	return *this;
}

Utility::Thr::Config &Utility::Thr::Config::name(const char *a)
{
	config.thread_name = a;
	return *this;
}

Utility::Thr::Config &Utility::Thr::Config::priority(int a)
{
	config.prio = a;
	return *this;
}

Utility::Thr::Config &Utility::Thr::Config::stack(int a)
{
	config.stack_size = a;
	return *this;
}

void Utility::Thr::FreertosTask::run(void *aInstance)
{
	auto &task = *reinterpret_cast<Thread *>(aInstance);
	task.run();
}

void Utility::Thr::FreertosTask::start()
{
	if (static_cast<int>(CorePin::CoreNone) == taskInfo.core) {
		xTaskCreate(FreertosTask::run, taskInfo.name, taskInfo.stack, this, taskInfo.prio, &taskInfo.handle);
	} else {
		xTaskCreatePinnedToCore(FreertosTask::run, taskInfo.name, taskInfo.stack, this, taskInfo.prio, &taskInfo.handle,
			taskInfo.core);
	}
}

void Utility::Thr::FreertosTask::suspend()
{
	vTaskSuspend(taskInfo.handle);
}

void Utility::Thr::FreertosTask::resume()
{
	vTaskResume(taskInfo.handle);
}

#include "utility/thr/Threading.hpp"

void Ut::Thr::setThreadCoreAffinity(int coreAffinity)
{
	auto cfg = esp_pthread_get_default_config();
	cfg.pin_to_core = coreAffinity;
	esp_pthread_set_cfg(&cfg);
}

Ut::Thr::Config::Config(bool aFromDefault): config{}
{
	if (!aFromDefault) {
		esp_pthread_get_cfg(&config);
	} else {
		config = esp_pthread_get_default_config();
	}
}

Ut::Thr::Config::~Config()
{
	esp_pthread_set_cfg(&config);
}

Ut::Thr::Config &Ut::Thr::Config::core(int a)
{
	config.pin_to_core = a;
	return *this;
}

Ut::Thr::Config &Ut::Thr::Config::inherit(bool a)
{
	config.inherit_cfg = a;
	return *this;
}

Ut::Thr::Config &Ut::Thr::Config::name(const char *a)
{
	config.thread_name = a;
	return *this;
}

Ut::Thr::Config &Ut::Thr::Config::priority(int a)
{
	config.prio = a;
	return *this;
}

Ut::Thr::Config &Ut::Thr::Config::stack(int a)
{
	config.stack_size = a;
	return *this;
}

void Ut::Thr::FreertosTask::run(void *aInstance)
{
	auto &task = *reinterpret_cast<Thread *>(aInstance);
	task.run();
}

void Ut::Thr::FreertosTask::start()
{
	if (static_cast<int>(CorePin::CoreNone) == taskInfo.core) {
		xTaskCreate(FreertosTask::run, taskInfo.name, taskInfo.stack, this, taskInfo.prio, &taskInfo.handle);
	} else {
		xTaskCreatePinnedToCore(FreertosTask::run, taskInfo.name, taskInfo.stack, this, taskInfo.prio, &taskInfo.handle,
			taskInfo.core);
	}
}

void Ut::Thr::FreertosTask::suspend()
{
	suspended = true;
	vTaskSuspend(taskInfo.handle);
}

void Ut::Thr::FreertosTask::resume()
{
	if (suspended) {
		suspended = false;
		vTaskResume(taskInfo.handle);
	}
}

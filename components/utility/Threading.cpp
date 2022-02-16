#include "utility/Threading.hpp"

void Utility::Threading::setThreadCoreAffinity(int coreAffinity)
{
	auto cfg = esp_pthread_get_default_config();
	cfg.pin_to_core = coreAffinity;
	esp_pthread_set_cfg(&cfg);
}

Utility::Threading::Config::Config(bool aFromDefault): config{}
{
	if (!aFromDefault) {
		esp_pthread_get_cfg(&config);
	} else {
		config = esp_pthread_get_default_config();
	}
}

Utility::Threading::Config::~Config()
{
	esp_pthread_set_cfg(&config);
}

Utility::Threading::Config &Utility::Threading::Config::core(int a)
{
	config.pin_to_core = a;
	return *this;
}

Utility::Threading::Config &Utility::Threading::Config::inherit(bool a)
{
	config.inherit_cfg = a;
	return *this;
}

Utility::Threading::Config &Utility::Threading::Config::name(const char *a)
{
	config.thread_name = a;
	return *this;
}

Utility::Threading::Config &Utility::Threading::Config::priority(int a)
{
	config.prio = a;
	return *this;
}

Utility::Threading::Config &Utility::Threading::Config::stack(int a)
{
	config.stack_size = a;
	return *this;
}

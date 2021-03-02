#include "utility/Run.hpp"

void Utility::setThreadCoreAffinity(int coreAffinity)
{
	auto cfg = esp_pthread_get_default_config();
	cfg.pin_to_core = coreAffinity;
	esp_pthread_set_cfg(&cfg);
}
//
// RoutingRules.cpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)

#include "RoutingRules.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
#include "utility/LogSection.hpp"
#include <esp_log.h>
#include <algorithm>

GS_UTILITY_LOGV_METHOD_SET_ENABLED(Bdg::RoutingRules, reduce, 0)
GS_UTILITY_LOGV_METHOD_SET_ENABLED(Bdg::RoutingRules, find, 0)

using namespace Bdg::RoutingRulesImpl;
static constexpr unsigned kRulesReserved = 16;

namespace Bdg {
namespace RoutingRulesImpl {

bool operator<(const Bdg::RoutingRulesImpl::ReductionRule &aLhs, const Bdg::RoutingRulesImpl::ReductionRule &aRhs)
{
	return aLhs.ruleTrigger < aRhs.ruleTrigger;
}

bool operator<(const Bdg::RoutingRulesImpl::ReductionRule &aLhs, const Bdg::RoutingRulesImpl::RuleTrigger &aRhs)
{
	return aLhs.ruleTrigger < aRhs;
}

}  // namespace RoutingRulesImpl

RoutingRules::RoutingRules() : mutex{}, reductionRules(kRulesReserved)
{
	reductionRules.clear();
}

/// \brief So-called "static" routing rules do not require functor-based inference, which is the case with relation
/// to dynamic ones.
///
bool RoutingRules::addStatic(const EndpointVariant &aOrigin, const EndpointVariant &aIntermediary,
	const EndpointVariant &aReduce)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	RoutingRulesImpl::ReductionRule reductionRule{{aOrigin, aIntermediary}, {aReduce}};

	auto it = find(aOrigin, aIntermediary);
	const bool ret = (std::end(reductionRules) == it);

	if (ret) {
		if (reductionRules.size() == reductionRules.capacity()) {
			ESP_LOGW(Bdg::kDebugTag, "RoutingRules::addStatic() storage capacity will be extended");
		}

		reductionRules.insert(std::lower_bound(std::begin(reductionRules), std::end(reductionRules), reductionRule),
			reductionRule);
		ESP_LOGI(Bdg::kDebugTag, "RoutingRules::addStatic() Added new rule:");
		aOrigin.logi("RoutingRules::addStatic() Origin:    ");
		aIntermediary.logi("RoutingRules::addStatic() Receiver:  ");
		aReduce.logi("RoutingRules::addStatic() Reduction: ");

	} else {
		ESP_LOGW(Bdg::kDebugTag, "RoutingRules::addStatic() attempt to add a duplicate rule");
	}

	return ret;
}

/// \brief Remove a rule using (source, destination) pair
///
void RoutingRules::remove(const EndpointVariant &aOrigin, const EndpointVariant &aIntermediary)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	auto it = find(aOrigin, aIntermediary);

	if (std::end(reductionRules) != it) {
		reductionRules.erase(it);
		ESP_LOGI(Bdg::kDebugTag, "RoutingRules::remove() REMOVED a rule:");
		aOrigin.logi("RoutingRules::remove() Origin:    ");
		aIntermediary.logi("RoutingRules::remove() Receiver:  ");
	} else {
		GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, RoutingRules, remove,
			"RoutingRules::remove() Could not find a rule");
	}
}

/// \brief Searches for a rule to be applied using a (FROM, TO) pair. If there is any, returns true, and assigns
/// `aoSrc` w/ the found endpoint (it's called reducing)
///
bool RoutingRules::reduce(EndpointVariant &aoSrc, const EndpointVariant &aCandidate)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	auto it = find(aoSrc, aCandidate);
	bool ret = (std::end(reductionRules) != it);

	GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, RoutingRules, reduce, "Rules count %d", reductionRules.size());

	if (ret) {  // The rule has been found, perform reduction
		aoSrc = it->reductionVariant;
	} else {
		GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, RoutingRules, reduce,
			"RoutingRules::reduce Could not find a suitable reduction");
	}

	return ret;
}

typename std::vector<RoutingRulesImpl::ReductionRule>::iterator RoutingRules::find(const EndpointVariant &aOrigin,
	const EndpointVariant &aIntermediary)
{
	const RuleTrigger ruleTrigger{aOrigin, aIntermediary};
	const auto end = std::end(reductionRules);
	auto it = std::lower_bound(std::begin(reductionRules), end, ruleTrigger);

	if (it != end) {
		if (it->ruleTrigger != ruleTrigger) {
			it = end;
		} else {
			GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, RoutingRules, find, "found a direct reduction");
		}
	}

	if (it == end) {
		for (auto &alternative : aOrigin.asAlternative()) {
			it = find(alternative, aIntermediary);

			if (std::end(reductionRules) != it) {
				GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, RoutingRules, find,
					"found an alternative reduction");
				break;
			}
		}
	}

	return it;
}

}  // namespace Bdg

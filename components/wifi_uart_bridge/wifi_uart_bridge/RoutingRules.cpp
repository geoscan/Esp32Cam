//
// RoutingRules.cpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "RoutingRules.hpp"
#include <algorithm>

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

RoutingRules::RoutingRules() : mutex{}, reductionRules{kRulesReserved}
{
}

bool RoutingRules::addStatic(const EndpointVariant &aOrigin, const EndpointVariant &aIntermediary,
	const EndpointVariant &aReduce)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	RoutingRulesImpl::ReductionRule reductionRule{{aOrigin, aIntermediary}, {aReduce}};

	auto it = find(aOrigin, aIntermediary);
	const bool ret = (std::end(reductionRules) == it);

	if (ret) {
		reductionRules.insert(std::lower_bound(std::begin(reductionRules), std::end(reductionRules), reductionRule),
			reductionRule);
	}

	return ret;
}

void RoutingRules::remove(const EndpointVariant &aOrigin, const EndpointVariant &aIntermediary)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	auto it = find(aOrigin, aIntermediary);

	if (std::end(reductionRules) != it) {
		reductionRules.erase(it);
	}
}


/// \brief Searches for a rule to be applied using a (FROM, TO) pair. If there is any, returns true, and assigns
/// `aoSrc` w/ the found rule
///
bool RoutingRules::reduce(EndpointVariant &aoSrc, const EndpointVariant &aCandidate)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	auto it = find(aoSrc, aCandidate);
	const bool ret = (std::end(reductionRules) != it);

	if (ret) {
		aoSrc = it->reductionVariant.match(
			[&aoSrc, &aCandidate](DynamicReduction &a) {return a(aoSrc, aCandidate); },
			[](const EndpointVariant &a) {return a; }
		);
	}

	return ret;
}

typename std::vector<RoutingRulesImpl::ReductionRule>::iterator RoutingRules::find(const EndpointVariant &aOrigin,
	const EndpointVariant &aIntermediary)
{
	const RuleTrigger ruleTrigger{aOrigin, aIntermediary};
	auto end = std::end(reductionRules);
	auto it = std::lower_bound(std::begin(reductionRules), end, ruleTrigger);

	if (it != end) {
		if (it->ruleTrigger != ruleTrigger) {
			it = end;
		}
	}

	return it;
}

/// \brief Searches for the rule comparing the first two positions
///

}  // namespace Bdg

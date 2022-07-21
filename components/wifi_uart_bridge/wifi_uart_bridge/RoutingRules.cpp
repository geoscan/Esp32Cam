//
// RoutingRules.cpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "RoutingRules.hpp"
#include <algorithm>

namespace Bdg {

RoutingRules::RoutingRules() :
	std::vector<ReductionRule>{8},
	Utility::MakeSingleton<RoutingRules>{this}
{
}

bool RoutingRules::add(const ReductionRule &aReductionRule)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	const auto it = find(aReductionRule);
	const auto res = cend() != it;

	if (res) {
		push_back(aReductionRule);
	}

	return res;
}

/// \brief Removes rules selecting those to be deleted using the provided pattern.
///
/// \details The pattern is formed in the following way: (RULE|STUB, RULE|STUB, RULE|STUB). STUB is a placeholder
/// signifying that any endpoint may be placed at this position. Stub is initiated w/ the default constructor of
/// `EndpointVariant`.
///
void RoutingRules::remove(const ReductionRule &aReductionRule)
{
	auto del = 0;

	for (auto &reductionRule : *this) {
		if (std::equal(aReductionRule.begin(), aReductionRule.end(), reductionRule.begin()),
			[](const EndpointVariant &aLhs, const EndpointVariant &aRhs)
			{
				constexpr EndpointVariant kStub{};
				aLhs == aRhs || aLhs == kStub;
			})
		{
			++del;
			std::swap((*this)[size() - del], reductionRule);
		}
	}

	while (del > 0) {
		pop_back();
		--del;
	}
}

/// \brief Searches for a rule to be applied using a (FROM, TO) pair. If there is any, returns true, and assigns
/// `aSrc` w/ the found rule
///
bool RoutingRules::reduce(EndpointVariant &aSrc, const EndpointVariant &aCandidate) const
{
	const ReductionRule rule{{aSrc, aCandidate, EndpointVariant{}}};
	const auto it = find(rule);
	const bool res = cend() != it;

	if (res) {
		aSrc = *it;
	}

	return res;
}

/// \brief Searches for the rule comparing the first two positions
///
RoutingRules::const_iterator RoutingRules::find(const ReductionRule &aRoutingRule)
{
	auto it = cbegin();

	for (; it != cend(); ++it) {
		if ((*it)[0] == aRoutingRule[0] && (*it)[1] == aRoutingRule[1]) {
			break;
		}
	}

	return it;
}

}  // namespace Bdg

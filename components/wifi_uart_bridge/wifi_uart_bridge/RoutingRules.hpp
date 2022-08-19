//
// RoutingRules.hpp
//
// Created on: Jul 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ROUTING_RULES_HPP_)
#define WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ROUTING_RULES_HPP_

#include "utility/MakeSingleton.hpp"
#include "wifi_uart_bridge/EndpointVariant.hpp"
#include <array>
#include <list>
#include <vector>
#include <mutex>
#include <functional>

namespace Bdg {

using DynamicReduction = std::function<EndpointVariant(const EndpointVariant &, const EndpointVariant &)>;

namespace RoutingRulesImpl {

using RuleTrigger = std::tuple<EndpointVariant, EndpointVariant>;  ///< Unique identifier of a rule
using ReductionVariant = EndpointVariant;

struct ReductionRule {
	RuleTrigger ruleTrigger;
	ReductionVariant reductionVariant;
};

bool operator<(const RoutingRulesImpl::ReductionRule &aLhs, const RoutingRulesImpl::ReductionRule &aRhs);
bool operator<(const RoutingRulesImpl::ReductionRule &aLhs, const RuleTrigger &aRhs);

}  // namespace RoutingRulesImpl

/// \brief Stores routing sequences.
///
/// \details A routing sequence is a triple of endpoints (FROM, TO, REDUCTION). FROM is an originator (or virtual
/// originator, more on that further), TO is a receiver (candidate), REDUCTION is a replacement for FROM after
/// forwarding.
///
/// Say, we have a set of rules {(A, B, R1), (R1, C, NONE), (A, D, NONE)}, and 3 endpoints: {A, B, C}.
/// If A is the originator, then the produced sequence is passed to B and D.
/// If B forwards (or transforms) the sequence, the reductioned endpoint is R1, and the potential receiver of the
/// sequence is C.
/// In this example, R1 is a virtual originator, since we have no endpoint called R1.
///
class RoutingRules : public Ut::MakeSingleton<RoutingRules> {
public:
	RoutingRules();
	bool addStatic(const EndpointVariant &, const EndpointVariant&, const EndpointVariant &);
	void remove(const EndpointVariant &, const EndpointVariant &);
	bool reduce(EndpointVariant &aoSrc, const EndpointVariant &aCandidate);

private:
	typename std::vector<RoutingRulesImpl::ReductionRule>::iterator find(const EndpointVariant &, const EndpointVariant &);

private:
	mutable std::mutex mutex;
	std::vector<RoutingRulesImpl::ReductionRule> reductionRules;
};

}  // namespace Bdg

#endif // WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ROUTING_RULES_HPP_

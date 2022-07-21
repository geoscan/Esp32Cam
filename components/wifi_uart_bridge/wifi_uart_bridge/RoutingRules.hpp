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
#include <mutex>

namespace Bdg {

using ReductionRule = typename std::array<EndpointVariant, 3>;  ///< (Rule A, Endpoint candidate, Rule B on forwarding). The first two positions constitute a unique identifier of a rule.

/// \brief Stores routing sequences.
///
/// \details A routing sequence is a triple of endpoints (FROM, TO, REDUCTION). FROM is an originator (or virtual
/// originator, more on that further), TO is a receiver (candidate), REDUCTION is a replacement for FROM after
/// forwarding.
///
/// Say, we have a set of rules {(A, B, R1), (R1, C, NONE), (A, D, NONE)}.
/// If A is the originator, then the produced sequence is passed to B and D.
/// If B forwards (or transforms) the sequence, the reductioned endpoint is R1, and the potential receiver of the
/// sequence is C.
///
class RoutingRules : public std::vector<ReductionRule>, public Utility::MakeSingleton<RoutingRules> {
public:
	RoutingRules();
	bool add(const ReductionRule &);
	void remove(const ReductionRule &);
	bool reduce(EndpointVariant &aoSrc, const EndpointVariant &aCandidate) const;

private:
	RoutingRules::const_iterator find(const ReductionRule &) const;

private:
	mutable std::mutex mutex;
};

}  // namespace Bdg

#endif // WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_ROUTING_RULES_HPP_

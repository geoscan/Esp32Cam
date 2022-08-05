//
// RaiiCounter.cpp
//
// Created on: Jul 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "RaiiCounter.hpp"
#include <algorithm>

namespace Utility {
namespace Comm {

RaiiCounter::RaiiCounter(unsigned &aOwners) : owners{&aOwners}
{
	++owners;
}

RaiiCounter::~RaiiCounter()
{
	if (nullptr != owners) {
		--(*owners);
	}
}

RaiiCounter::RaiiCounter(const RaiiCounter &aRaiiCounter) : owners{const_cast<unsigned *>(aRaiiCounter.owners)}
{
	if (nullptr != owners) {
		++(*owners);
	}
}

RaiiCounter::RaiiCounter(RaiiCounter &&aRaiiCounter) : owners{nullptr}
{
	std::swap(owners, aRaiiCounter.owners);
}

RaiiCounter &RaiiCounter::operator=(const RaiiCounter &aRaiiCounter)
{
	if (nullptr != owners) {
		--(*owners);
	}

	owners = const_cast<unsigned *>(aRaiiCounter.owners);

	if (nullptr != owners) {
		++(*owners);
	}

	return *this;
}

RaiiCounter &RaiiCounter::operator=(RaiiCounter &&aRaiiCounter)
{
	if (nullptr != owners) {
		--(*owners);
		owners = nullptr;
	}

	std::swap(aRaiiCounter.owners, owners);

	return *this;
}

void RaiiCounter::reset()
{
	owners = nullptr;
}

unsigned RaiiCounter::getValue() const
{
	if (nullptr == owners) {
		return kInvalidValue;
	} else {
		return *owners;
	}
}

}  // namespace Comm
}  // namespace Utility

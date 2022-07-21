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

RaiiCounter RaiiCounter::clone()
{
	return RaiiCounter{*owners};
}

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

RaiiCounter::RaiiCounter(RaiiCounter &&aRaiiCounter) : owners{nullptr}
{
	std::swap(owners, aRaiiCounter.owners);
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

}  // namespace Comm
}  // namespace Utility

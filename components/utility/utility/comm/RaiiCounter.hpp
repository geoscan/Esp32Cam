//
// RaiiCounter.hpp
//
// Created on: Jul 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_COMM_RAIICOUNTER_HPP_)
#define UTILITY_UTILITY_COMM_RAIICOUNTER_HPP_

namespace Utility {
namespace Comm {

struct RaiiCounter final {
public:
	RaiiCounter clone();
	RaiiCounter(unsigned &aOwners);
	~RaiiCounter();
	RaiiCounter(const RaiiCounter &) = delete;
	RaiiCounter(RaiiCounter &&);
	RaiiCounter &operator=(const RaiiCounter &) = delete;
	RaiiCounter &operator=(RaiiCounter &&);
private:
	unsigned *owners;
};


}  // namespace Comm
}  // namespace Utility

#endif // UTILITY_UTILITY_COMM_RAIICOUNTER_HPP_

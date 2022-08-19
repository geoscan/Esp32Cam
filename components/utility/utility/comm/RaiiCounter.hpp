//
// RaiiCounter.hpp
//
// Created on: Jul 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_COMM_RAIICOUNTER_HPP_)
#define UTILITY_UTILITY_COMM_RAIICOUNTER_HPP_

#include <limits>

namespace Ut {
namespace Comm {

struct RaiiCounter final {
public:
	static constexpr auto kInvalidValue = std::numeric_limits<unsigned>::max();

	RaiiCounter(unsigned &aOwners);
	~RaiiCounter();
	RaiiCounter(const RaiiCounter &);
	RaiiCounter(RaiiCounter &&);
	RaiiCounter &operator=(const RaiiCounter &);
	RaiiCounter &operator=(RaiiCounter &&);
	void reset();
	unsigned getValue() const;
private:
	unsigned *owners;
};


}  // namespace Comm
}  // namespace Ut

#endif // UTILITY_UTILITY_COMM_RAIICOUNTER_HPP_

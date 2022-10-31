//
// Tracking.hpp
//
// Created on: Oct 31, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(SUB_INCLUDE_SUB_TRACKING_HPP_)
#define SUB_INCLUDE_SUB_TRACKING_HPP_

#include "sub/Subscription.hpp"

namespace Sub {
namespace Trk {

/// \brief Encapsulates tracking info which will be distributed among receivers.
struct MosseTrackerUpdate {
	// Frame
	int frameHeight;
	int frameWidth;
	// Roi
	int roiX;
	int roiY;
	int roiWidth;
	int roiHeight;
	/// Peak-to-sidelobe ratio
	float psr;
};

using OnMosseTrackerUpdate = IndKey<void(MosseTrackerUpdate)>;

}  // namespace Trk
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_TRACKING_HPP_

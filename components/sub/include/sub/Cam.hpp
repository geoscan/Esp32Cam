//
// Cam.hpp
//
// Created on: Apr 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SUB_INCLUDE_SUB_CAM_HPP_
#define SUB_INCLUDE_SUB_CAM_HPP_

#include "sub/Subscription.hpp"

namespace Sub {
namespace Cam {
namespace Topic {
struct Shot;
struct RecordStart;
struct RecordStop;
}  // namespace Topic

using ShotFile = Sub::NoLockKey<bool(const char *), Topic::Shot>;
using RecordStart = Sub::NoLockKey<void(const char *), Topic::RecordStop>;
using RecordStop = Sub::NoLockKey<void(), Topic::RecordStop>;

}  // namespace Rout
}  // namespace Cam

#endif // SUB_INCLUDE_SUB_CAM_HPP_

//
// Profile.cpp
//
// Created on: Sep 30, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Mosse.hpp"
#include "module/ModuleBase.hpp"
#include "tracking/tracking.hpp"
#include <esp_log.h>
#include "Profile.hpp"

namespace Trk {

Profile::Profile() :
	key{{&Profile::onFrame, this}},
	tracker{Mosse::getFp16AbRawF32BufDynAlloc()},
	state{State::CamConfStart}
{
}

void Profile::onFrame(const std::shared_ptr<Cam::Frame> &aFrame)
{
	// TODO: push into work queue, probably will cause stack overflow
	switch (state) {
		case State::CamConfStart {  // Initialize the camera, switch it to grayscale mode
			Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>("grayscale",
				[&state](const Mod::Fld::WriteResp &aWriteResp)
				{
					if (aWriteResp.isOk()) {
						ESP_LOGI(Trk::kDebugTag, "Profile: switched camera to grayscale mode");
						state = State::TrackerInit;
					} else {
						ESP_LOGE(Trk::kDebugTag, "Profile: failed to switch the camera to grayscale mode %s",
							aWriteResp.resultAsCstr());
						state = State::CamConfFailed;
					}
				});

			break;
		}
		case State::TrackerInit: {
			if (static_cast<bool>(aFrame)) {
				Mosse::Tp::Image image{aFrame.get()->data(), aFrame.get()->height(), aFrame.get()->width()};
				Mosse::Tp::Roi roi{{0, 0}, {64, 64}};  // TODO Missing frame size bounds check
				tracker.init(image, roi);
				state = State::TrackerRunning;
				ESP_LOGI(Trk::kDebugTag, "Profile: initialized tracker");
			} else {
				ESP_LOGW(Trk::kDebugTag, "Profile: nullptr frame");
			}
		}
		case State::TrackerRunning: {
			if (static_cast<bool>(aFrame)) {
				Mosse::Tp::Image image{aFrame.get()->data(), aFrame.get()->height(), aFrame.get()->width()};
				tracker.update(image, true);
				ESP_LOGI(Trk::kDebugTag, "Profile: psr %.4f", tracker.lastPsr());
			}
		}
		default:
			break;
	}
}

}  // namespace Trk

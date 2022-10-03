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
#include "Thread.hpp"

namespace Trk {

Mosse::Port::Thread &mosseThreadApi()
{
	static Thread thr;

	return thr;
}

static constexpr std::size_t knMosseThreads = 2;

Profile::Profile() :
	tracker{Mosse::getFp16AbRawF32BufDynAllocThreaded(mosseThreadApi(), 1)},
	state{State::CamConfStart},
	key{{&Profile::onFrame, this}}
{
}

void Profile::onFrame(const std::shared_ptr<Cam::Frame> &aFrame)
{
	assert(nullptr != aFrame.get());
	// TODO: push into work queue, probably will cause stack overflow
	switch (state) {
		case State::CamConfStart: {  // Initialize the camera, switch it to grayscale mode
			Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>("grayscale",
				[this](const Mod::Fld::WriteResp &aWriteResp)
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
				Mosse::Tp::Image image{static_cast<std::uint8_t *>(aFrame.get()->data()), aFrame.get()->height(),
					aFrame.get()->width()};
				Mosse::Tp::Roi roi{{0, 0}, {64, 64}};  // TODO Missing frame size bounds check
				tracker.init(image, roi);
				state = State::TrackerRunning;
				ESP_LOGI(Trk::kDebugTag, "Profile: initialized tracker");
			} else {
				ESP_LOGW(Trk::kDebugTag, "Profile: nullptr frame");
			}

			break;
		}
		case State::TrackerRunning: {
			if (static_cast<bool>(aFrame)) {
				Mosse::Tp::Image image{static_cast<std::uint8_t *>(aFrame.get()->data()), aFrame.get()->height(),
					aFrame.get()->width()};
				tracker.update(image, true);
				ESP_LOGI(Trk::kDebugTag, "Profile: psr %.4f", tracker.lastPsr());
			}

			break;
		}
		default:
			break;
	}
}

}  // namespace Trk

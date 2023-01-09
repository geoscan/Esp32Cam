#include <esp_err.h>
#include <esp_http_server.h>

#include "pages.h"
#include "Ov2640.hpp"
#include "sub/Subscription.hpp"
#include "utility/time.hpp"
#include <chrono>
#include <cstdint>
#include <functional>

/// Stateful camera subscriber.
static struct CameraSubscriber {

	struct OnFrameHook {
		std::mutex mutex;
		std::function<void(const std::shared_ptr<Cam::Frame> &)> callback;
	};

	/// If a subscription is active, gets triggered on a new frame posted into
	/// the queue.
	void onFrame(const std::shared_ptr<Cam::Frame> &frame)
	{
		std::lock_guard<std::mutex> lock{onFrameHook.mutex};

		if (static_cast<bool>(onFrameHook.callback)) {
			onFrameHook.callback(frame);
			onFrameHook.callback = nullptr;
		}
	}

	bool isReceived()
	{
		std::lock_guard<std::mutex> lock{onFrameHook.mutex};
		const bool res = !static_cast<bool>(onFrameHook.callback);

		return res;
	}

	CameraSubscriber() : keyNewFrame{&CameraSubscriber::onFrame, this, false}
	{
	}

	template <class C>
	void waitFrameSync(C &&cbOnFrame)
	{
		{
			std::lock_guard<std::mutex> lock{onFrameHook.mutex};
			onFrameHook.callback = std::move(cbOnFrame);
		}
		keyNewFrame.enableSubscribe(true);
		const std::chrono::milliseconds timeEnd = std::chrono::milliseconds{Ut::bootTimeUs() / 1000 + kFrameTimeout};

		// Frame delay
		while (!isReceived() && std::chrono::milliseconds(Ut::bootTimeUs() / 1000) < timeEnd) {
			vTaskDelay(1);
		}
		keyNewFrame.enableSubscribe(false);
	}

	static constexpr std::size_t kFrameTimeout = 500;
	Sub::Key::NewFrame keyNewFrame;
	OnFrameHook onFrameHook;
} sCameraSubscriber{};

extern "C" esp_err_t cameraDemoHandler(httpd_req_t *req)
{
	esp_err_t res;
	res = httpd_resp_set_type(req, "image/jpeg");

	if (res == ESP_OK) {
		httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
	}

	res = ESP_FAIL;
	sCameraSubscriber.waitFrameSync(
		[&req, &res](const std::shared_ptr<Cam::Frame> &frame)
		{
			res = httpd_resp_send(req, static_cast<const char *>(frame->data()), frame->size());
		});

	if (res != ESP_OK) {
		httpd_resp_send(req, "", 0);
	}

	return res;
}

#include <sdkconfig.h>
#include <algorithm>
#include <nvs_flash.h>
#include <nvs_handle.hpp>
#include <array>
#include "Ov2640.hpp"
#include "esp_camera.h"
#include "utility/system/NvsWrap.hpp"
#include "sub/Cam.hpp"

using namespace std;

// ------------ Ov2640 ------------ //

static constexpr const char *kTag = "[OV2640]";
static constexpr const char *kNvsKey = "Ov2640";
static constexpr const char *kNvsFrameSize = "FrameSize";

/// \brief Memory constraints necessitate the use of frame size limitations
///
/// \details Format: (pixformat, "lt" limitation)
///
static constexpr std::array<std::tuple<pixformat_t, framesize_t>, 2> kResolutionLimit{{
	{PIXFORMAT_JPEG, FRAMESIZE_SVGA},
	{PIXFORMAT_GRAYSCALE, FRAMESIZE_QVGA}
}};

/// \brief Finds max. frame size for a pixformat being used currently.
///
static framesize_t pixformatToResolutionLimit(pixformat_t aPixformat)
{
	static constexpr framesize_t kFramesizeDefault = FRAMESIZE_240X240;

	for (const auto &fmtSzPair : kResolutionLimit) {
		if (std::get<0>(fmtSzPair) == aPixformat) {
			return std::get<1>(fmtSzPair);
		}
	}

	return kFramesizeDefault;
}

using FrameSizeModeMap = std::pair<int, int>;

static constexpr std::array<FrameSizeModeMap, 22> kFrameSizeModeMap {{
	{96, 96},
	{160, 120},
	{176, 144},
	{240, 176},
	{240, 240},
	{320, 240},
	{400, 296},
	{480, 320},
	{640, 480},
	{800, 600},
	{1024, 768},
	{1280, 720},
	{1280, 1024},
	{1600, 1200},
	{1920, 1080},
	{720, 1280},
	{864, 1536},
	{2048, 1536},
	{2560, 1440},
	{2560, 1600},
	{1080, 1920},
	{2560, 1920},
}};

/// \brief Maps pixformat ID to a human-readable representation
///
static constexpr std::array<std::tuple<pixformat_t, const char *>, 2> kFramePixformat {{
	{PIXFORMAT_JPEG, "jpeg"},
	{PIXFORMAT_GRAYSCALE, "grayscale"},
}};

/// \brief Converts pixformat to a human-readable representation
///
static const char *pixformatToStr(pixformat_t aPixformat)
{
	for (const auto &format : kFramePixformat) {
		if (std::get<0>(format) == aPixformat) {
			return std::get<1>(format);
		}
	}

	return "";
}

Ov2640::Ov2640() :
	Mod::ModuleBase{Mod::Module::Camera}
{
	esp_camera_add_hook_on_frame(hookOnFrame);  // The way it implemented, the hook will just be replaced, so it safe to set a hook without any checks
}

void Ov2640::init()
{
	cameraConfigLoad();
	cameraConfig.pixel_format = status.pixformat;
	auto resolutionLimit = pixformatToResolutionLimit(status.pixformat);

	// Guardrails to prevent buffer allocation failures
	if (cameraConfig.frame_size >= resolutionLimit) {
		cameraConfig.frame_size = static_cast<framesize_t>(resolutionLimit - 1);
	}

	status.initialized = (esp_camera_init(&cameraConfig) == ESP_OK);
	status.frame.w = std::get<0>(kFrameSizeModeMap[cameraConfig.frame_size]);
	status.frame.h = std::get<1>(kFrameSizeModeMap[cameraConfig.frame_size]);
}

/// \brief Load
void Ov2640::cameraConfigLoad()
{
	// WARNING: The following only represents pinout of ESP32 Ai Thinker
	enum {
		CAM_PIN_PWDN  = 32, // Power down is used
		CAM_PIN_RESET = -1, // Software reset will be performed
		CAM_PIN_XCLK  =  0,
		CAM_PIN_SIOD  = 26,
		CAM_PIN_SIOC  = 27,
		CAM_PIN_D7    = 35,
		CAM_PIN_D6    = 34,
		CAM_PIN_D5    = 39,
		CAM_PIN_D4    = 36,
		CAM_PIN_D3    = 21,
		CAM_PIN_D2    = 19,
		CAM_PIN_D1    = 18,
		CAM_PIN_D0    =  5,
		CAM_PIN_VSYNC = 25,
		CAM_PIN_HREF  = 23,
		CAM_PIN_PCLK  = 22,
	};

	cameraConfig = camera_config_t{
		.pin_pwdn     = CAM_PIN_PWDN,
		.pin_reset    = CAM_PIN_RESET,
		.pin_xclk     = CAM_PIN_XCLK,
		.pin_sscb_sda = CAM_PIN_SIOD,
		.pin_sscb_scl = CAM_PIN_SIOC,

		.pin_d7    = CAM_PIN_D7,
		.pin_d6    = CAM_PIN_D6,
		.pin_d5    = CAM_PIN_D5,
		.pin_d4    = CAM_PIN_D4,
		.pin_d3    = CAM_PIN_D3,
		.pin_d2    = CAM_PIN_D2,
		.pin_d1    = CAM_PIN_D1,
		.pin_d0    = CAM_PIN_D0,
		.pin_vsync = CAM_PIN_VSYNC,
		.pin_href  = CAM_PIN_HREF,
		.pin_pclk  = CAM_PIN_PCLK,

		//XCLK 20MHz or 10MHz for Ov2640 double FPS (Experimental)
		.xclk_freq_hz = 20000000,
		.ledc_timer   = LEDC_TIMER_0,
		.ledc_channel = LEDC_CHANNEL_0,

		.pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
		.frame_size   = FRAMESIZE_HVGA,  // QQVGA-UXGA Do not use sizes above QVGA when not JPEG

		.jpeg_quality = 12, //0-63 lower number means higher quality
		.fb_count     = 1,  //if more than one, i2s runs in continuous mode. Use only with JPEG
		.n_managed_buffers = 0
	};

	std::uint8_t frame;
	esp_err_t err = Ut::Sys::nvsGet(kNvsKey, kNvsFrameSize, frame);

	if (ESP_OK == err) {
		const auto resolutionLimit = pixformatToResolutionLimit(status.pixformat);

		if (frame < resolutionLimit) {
			cameraConfig.frame_size = static_cast<framesize_t>(frame);
		} else {
			cameraConfig.frame_size = resolutionLimit;
			ESP_LOGW(kTag, "Unsupported frame size %d", static_cast<int>(cameraConfig.frame_size));
		}
	}
}

void Ov2640::reinit()
{
	esp_camera_deinit();
	init();
}

#if CONFIG_OV2640_CUSTOM_BUFFER_MANAGEMENT
std::shared_ptr<Cam::Frame> Ov2640::getFrame()
{
	static std::array<std::weak_ptr<Cam::Frame>, CONFIG_OV2640_CUSTOM_BUFFER_MANAGEMENT_N_BUFFERS> wpBuffers;
	static size_t usedBuffers = 0;

	// Find a buffer which is not being used at the moment

	for (size_t i = 0, nUsed = 0; i < wpBuffers.size(); ++i) {
		if (i > 0) {
			ESP_LOGI(kTag, "using additional buffers");
		}
		if (wpBuffers[i].expired()) {

			auto frameBuffer = esp_camera_nfb_get(i);
			if (!frameBuffer) {
				continue;
			}

			std::shared_ptr<Ov2640::Frame> sp(new Ov2640::Frame(frameBuffer));
			wpBuffers[i] = sp;

			return sp;
		} else {
			++nUsed;
			if (nUsed != usedBuffers) {
				usedBuffers = nUsed;
				ESP_LOGI(kTag, "# used buffers: %d", usedBuffers);
			}
		}
	}
	ESP_LOGW(kTag, "could not find a free buffer");

	return std::make_shared<Cam::Frame>();
}
#else
std::shared_ptr<Cam::Frame> Ov2640::getFrame()
{
	auto fb = esp_camera_fb_get();

	if (!fb) {
		ESP_LOGW(kTag, "Got nullptr frame");
		return std::make_shared<Cam::Frame>();
	}

	return std::shared_ptr<Cam::Frame>(new Ov2640::Frame(fb));
}

#endif

void Ov2640::getFieldValue(Mod::Fld::Req aRequest, Mod::Fld::OnResponseCallback aOnResponse)
{
	using namespace Mod;

	switch (aRequest.field) {
		case Fld::Field::FrameSize:
			if (status.initialized) {
				aOnResponse(makeResponse<Module::Camera, Fld::Field::FrameSize>(status.frame.w, status.frame.h));
			}

			break;

		case Fld::Field::Initialized:
			aOnResponse(makeResponse<Module::Camera, Fld::Field::Initialized>(status.initialized));

			break;

		case Fld::Field::ModelName:
			if (status.initialized) {
				aOnResponse(makeResponse<Module::Camera, Fld::Field::ModelName>("OV2640"));
			}

			break;

		case Fld::Field::VendorName:
			if (status.initialized) {
				aOnResponse(makeResponse<Module::Camera, Fld::Field::VendorName>("OmniVision"));
			}

			break;

		case Fld::Field::FrameFormat: {
			if (status.initialized) {
				aOnResponse(makeResponse<Module::Camera, Fld::Field::FrameFormat>(pixformatToStr(status.pixformat)));
			}

			break;
		}
		default:
			break;

	}
}

void Ov2640::setFieldValue(Mod::Fld::WriteReq aReq, Mod::Fld::OnWriteResponseCallback aCb)
{
	switch (aReq.field) {
		case Mod::Fld::Field::FrameSize: {
			const auto frameSize = aReq.variant.getUnchecked<Mod::Module::Camera, Mod::Fld::Field::FrameSize>();
			std::uint8_t mode = 0;
			bool mapped = false;

			for (; mode <= kFrameSizeModeMap.size(); ++mode) {
				if (kFrameSizeModeMap[mode] == frameSize) {
					mapped = true;

					break;
				}
			}

			if (mapped) {
				const auto resolutionLimit = pixformatToResolutionLimit(status.pixformat);

				if (mode < resolutionLimit) {
					auto err = Ut::Sys::nvsSet(kNvsKey, kNvsFrameSize, mode);

					if (err != ESP_OK) {
						aCb({Mod::Fld::RequestResult::StorageError});
						ESP_LOGW(kTag, "Unable to save frame size value to NVS, error \"%s\"", esp_err_to_name(err));
					} else {
						aCb({Mod::Fld::RequestResult::Ok});
						reinit();
					}
				} else {
					aCb({Mod::Fld::RequestResult::OutOfRange});
					ESP_LOGW(kTag, "Resolution %dx%d exceeds threshold %dx%d", std::get<0>(frameSize),
						std::get<1>(frameSize), std::get<0>(kFrameSizeModeMap[resolutionLimit - 1]),
						std::get<1>(kFrameSizeModeMap[resolutionLimit - 1]));
				}
			} else {
				ESP_LOGW(kTag, "Incompatible frame size %dx%d", std::get<0>(frameSize), std::get<1>(frameSize));
				aCb({Mod::Fld::RequestResult::OutOfRange});
			}

			break;
		}
		case Mod::Fld::Field::FrameFormat: {  // Change current camera mode and reconfigure the sensor
			const char *frameFormatStr = aReq.variant.getUnchecked<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>();

			for (auto &format : kFramePixformat) {
				if (strcmp(std::get<1>(format), frameFormatStr) == 0) {
					status.pixformat = std::get<0>(format);
					aCb({Mod::Fld::RequestResult::Ok});
					reinit();
					return;
				}
			}

			aCb({Mod::Fld::RequestResult::Other, "Unsupported pixframe"});

		}
		default:
			break;
	}
}

/// \brief Invokes subscribers on camera frames that a new frame has been received
///
void Ov2640::hookOnFrame(camera_fb_t *aFrame)
{
	static Sub::Key::NewFrame key;
	std::shared_ptr<Cam::Frame> frame{new Ov2640::Frame{aFrame}};
	key.notify(frame);
}

// ------------ Ov2640::Frame ------------ //

Ov2640::Frame::Frame(camera_fb_t *aFb) : Cam::Frame(aFb->buf, aFb->len), fb(aFb)
{
}

Ov2640::Frame::Frame(Frame &&frame) : Cam::Frame(std::move(frame)), fb(frame.fb)
{
	frame.fb = nullptr;
}

Ov2640::Frame &Ov2640::Frame::operator=(Frame &&frame)
{
	*static_cast<Cam::Frame *>(this) = std::move(frame);
	fb = frame.fb;
	frame.fb = nullptr;
	return *this;
}

Ov2640::Frame::~Frame()
{
	if (fb) {
		esp_camera_fb_return(fb);
	}
}

int Ov2640::Frame::width()
{
	return fb->width;
}

int Ov2640::Frame::height()
{
	return fb->height;
}

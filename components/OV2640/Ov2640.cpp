#include <sdkconfig.h>
#include <algorithm>
#include <nvs_flash.h>
#include <array>
#include "Ov2640.hpp"
#include "esp_camera.h"

using namespace std;

// ------------ Ov2640 ------------ //

static constexpr const char *kTag = "[OV2640]";
static constexpr const char *kNvsKey = "Ov2640";
static constexpr const char *kNvsFrameSize = "FrameSize";
static constexpr auto kResolutionLimit = FRAMESIZE_VGA;

struct FrameSizeModeMap {
	framesize_t mode;
	std::uint16_t w;
	std::uint16_t h;
};

static constexpr std::array<FrameSizeModeMap, 22> kFrameSizeModeMap {{
	{FRAMESIZE_96X96, 96, 96},
	{FRAMESIZE_QQVGA, 160, 120},
	{FRAMESIZE_QCIF, 176, 144},
	{FRAMESIZE_HQVGA, 240, 176},
	{FRAMESIZE_240X240, 240, 240},
	{FRAMESIZE_QVGA, 320, 240},
	{FRAMESIZE_CIF, 400, 296},
	{FRAMESIZE_HVGA, 480, 320},
	{FRAMESIZE_VGA, 640, 480},
	{FRAMESIZE_SVGA, 800, 600},
	{FRAMESIZE_XGA, 1024, 768},
	{FRAMESIZE_HD, 1280, 720},
	{FRAMESIZE_SXGA, 1280, 1024},
	{FRAMESIZE_UXGA, 1600, 1200},
	{FRAMESIZE_FHD, 1920, 1080},
	{FRAMESIZE_P_HD, 720, 1280},
	{FRAMESIZE_P_3MP, 864, 1536},
	{FRAMESIZE_QXGA, 2048, 1536},
	{FRAMESIZE_QHD, 2560, 1440},
	{FRAMESIZE_WQXGA, 2560, 1600},
	{FRAMESIZE_P_FHD, 1080, 1920},
	{FRAMESIZE_QSXGA, 2560, 1920},
}};

Ov2640::Ov2640() :
	Mod::ModuleBase{Mod::Module::Camera}
{
}

void Ov2640::init()
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

	camera_config_t cameraConfig = {
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

#if CONFIG_OV2640_CUSTOM_BUFFER_MANAGEMENT
		.n_managed_buffers = CONFIG_OV2640_CUSTOM_BUFFER_MANAGEMENT_N_BUFFERS,  // Use manual buffer management (n_managed_buffers > 0)
#else
		.n_managed_buffers = 0,
#endif
	};

	{
		nvs_handle_t nvsHandle;
		auto err = nvs_open(kNvsKey, NVS_READONLY, &nvsHandle);

		if (err == ESP_OK) {
			std::uint8_t frame{};
			err = nvs_get_u8(nvsHandle, kNvsFrameSize, &frame);

			if (err == ESP_OK) {
				if (frame <= kResolutionLimit) {
					cameraConfig.frame_size = static_cast<framesize_t>(frame);
				} else {
					ESP_LOGW(kTag, "Unsupported frame size %d", static_cast<int>(cameraConfig.frame_size));
				}
			} else {
				ESP_LOGW(kTag, "Error when reading value \"%s\" \"%s\"", kNvsFrameSize, esp_err_to_name(err));
			}
		} else {
			ESP_LOGW(kTag, "Error when accessing NVS \"%s\"", esp_err_to_name(err));
		}
	}

	status.initialized = (esp_camera_init(&cameraConfig) == ESP_OK);

	for (const auto &map : kFrameSizeModeMap) {
		if (map.mode == cameraConfig.frame_size) {
			status.frame.w = map.w;
			status.frame.h = map.h;
		}
	}
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

		default:
			break;

	}
}

void Ov2640::setFieldValue(Mod::Fld::WriteReq aReq, Mod::Fld::OnWriteResponseCallback aCb)
{
	switch (aReq.field) {
		case Mod::Fld::Field::FrameSize: {
			const auto frameSize = aReq.variant.getUnchecked<Mod::Module::Camera, Mod::Fld::Field::FrameSize>();
			nvs_handle_t nvsHandle{};
			esp_err_t err = nvs_open(kNvsKey, NVS_READWRITE, &nvsHandle);

			if (err == ESP_OK) {
				const auto it = std::find_if(kFrameSizeModeMap.begin(), kFrameSizeModeMap.end(),
					[&frameSize](const FrameSizeModeMap &aMap) {
						return aMap.w == std::get<0>(frameSize) && aMap.h == std::get<1>(frameSize);
					});

				if (kFrameSizeModeMap.end() != it) {
					if (it->mode <= kResolutionLimit) {
						err = nvs_set_u8(nvsHandle, kNvsFrameSize, static_cast<std::uint8_t>(it->mode));

						if (err != ESP_OK) {
							aCb({Mod::Fld::RequestResult::StorageError});
							ESP_LOGW(kTag, "Unable to save frame size value to NVS, error \"%s\"", esp_err_to_name(err));
						} else {
							aCb({Mod::Fld::RequestResult::Ok});
						}
					} else {
						aCb({Mod::Fld::RequestResult::OutOfRange});
						ESP_LOGW(kTag, "Resolution %dx%d exceeds threshold", it->w, it->h);
					}
				} else {
					ESP_LOGW(kTag, "Incompatible frame size %dx%d", std::get<0>(frameSize), std::get<1>(frameSize));
					aCb({Mod::Fld::RequestResult::OutOfRange});
				}
			} else {
				ESP_LOGW(kTag, "Unable to open NVS storage, error \"%s\"", esp_err_to_name(err));
				aCb({Mod::Fld::RequestResult::StorageError});
			}

			break;
		}
		default:
			break;
	}
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

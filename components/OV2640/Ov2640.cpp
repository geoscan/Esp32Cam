#include <sdkconfig.h>
#include <algorithm>
#include "Ov2640.hpp"
#include "esp_camera.h"

using namespace std;

// ------------ Ov2640 ------------ //

static constexpr const char *kTag = "[OV2640]";


Ov2640::Ov2640() :
	Utility::Mod::ModuleBase{Utility::Mod::Module::Camera}
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

	status.initialized = (esp_camera_init(&cameraConfig) == ESP_OK);

	switch (cameraConfig.frame_size) {
		case FRAMESIZE_96X96:
			status.frame.w = 96;
			status.frame.h = 96;

			break;

		case FRAMESIZE_QQVGA:
			status.frame.w = 160;
			status.frame.h = 120;

			break;

		case FRAMESIZE_QCIF:
			status.frame.w = 176;
			status.frame.h = 144;

			break;

		case FRAMESIZE_HQVGA:
			status.frame.w = 240;
			status.frame.h = 176;

			break;

		case FRAMESIZE_240X240:
			status.frame.w = 240;
			status.frame.h = 240;

			break;

		case FRAMESIZE_QVGA:
			status.frame.w = 320;
			status.frame.h = 240;

			break;

		case FRAMESIZE_CIF:
			status.frame.w = 400;
			status.frame.h = 296;

			break;

		case FRAMESIZE_HVGA:
			status.frame.w = 480;
			status.frame.h = 320;

			break;

		case FRAMESIZE_VGA:
			status.frame.w = 640;
			status.frame.h = 480;

			break;

		case FRAMESIZE_SVGA:
			status.frame.w = 800;
			status.frame.h = 600;

			break;

		case FRAMESIZE_XGA:
			status.frame.w = 1024;
			status.frame.h = 768;

			break;

		case FRAMESIZE_HD:
			status.frame.w = 1280;
			status.frame.h = 720;

			break;

		case FRAMESIZE_SXGA:
			status.frame.w = 1280;
			status.frame.h = 1024;

			break;

		case FRAMESIZE_UXGA:
			status.frame.w = 1600;
			status.frame.h = 1200;

			break;

		case FRAMESIZE_FHD:
			status.frame.w = 1920;
			status.frame.h = 1080;

			break;

		case FRAMESIZE_P_HD:
			status.frame.w = 720;
			status.frame.h = 1280;

			break;

		case FRAMESIZE_P_3MP:
			status.frame.w = 864;
			status.frame.h = 1536;

			break;

		case FRAMESIZE_QXGA:
			status.frame.w = 2048;
			status.frame.h = 1536;

			break;

		case FRAMESIZE_QHD:
			status.frame.w = 2560;
			status.frame.h = 1440;

			break;

		case FRAMESIZE_WQXGA:
			status.frame.w = 2560;
			status.frame.h = 1600;

			break;

		case FRAMESIZE_P_FHD:
			status.frame.w = 1080;
			status.frame.h = 1920;

			break;

		case FRAMESIZE_QSXGA:
			status.frame.w = 2560;
			status.frame.h = 1920;

			break;

		default:

			break;
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

void Ov2640::getFieldValue(Utility::Mod::Fld::Req aRequest, Utility::Mod::Fld::OnResponseCallback aOnResponse)
{
	using namespace Utility::Mod;

	switch (aRequest.field) {
		case Fld::Field::FrameSize:
			aOnResponse(makeResponse<Module::Camera, Fld::Field::FrameSize>(status.frame.w, status.frame.h));

			break;

		case Fld::Field::Initialized:
			aOnResponse(makeResponse<Module::Camera, Fld::Field::Initialized>(status.initialized));

			break;

		case Fld::Field::ModelName:
			aOnResponse(makeResponse<Module::Camera, Fld::Field::ModelName>("OV2640"));

			break;

		case Fld::Field::VendorName:
			aOnResponse(makeResponse<Module::Camera, Fld::Field::VendorName>("OmniVision"));

			break;

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

#include "Ov2640.hpp"
#include "esp_camera.h"
#include <algorithm>
#include "CameraLock.hpp"

using namespace std;

bool Ov2640::isInit{false};

Ov2640::Ov2640()
{
	CamOv2640::CameraLock lock;
	init();
}

Ov2640::~Ov2640()
{
	CamOv2640::CameraLock lock;
	esp_camera_deinit();
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
		.frame_size   = FRAMESIZE_VGA,  // QQVGA-UXGA Do not use sizes above QVGA when not JPEG

		.jpeg_quality = 12, //0-63 lower number means higher quality
		.fb_count     = 1  //if more than one, i2s runs in continuous mode. Use only with JPEG
	};

	//
	isInit = esp_camera_init(&cameraConfig) == ESP_OK ? true : false;
}

Ov2640 &Ov2640::instance()
{
	static Ov2640 inst;
	return inst;
}

std::unique_ptr<Ov2640::Image> Ov2640::jpeg(/*JpegQuality quality*/)
{
	static constexpr uint8_t kJpegQuality = 80;
	CamOv2640::CameraLock    lock;
	unique_ptr<Image>        imgPtr(new Image());
	Image                    &img = *imgPtr;

	if (!isInit) {
		return {};
	}

	img.frameBuffer = esp_camera_fb_get();

	if (!img.frameBuffer) {
		return {};
	}

	if (img.frameBuffer->format == PIXFORMAT_JPEG) {
		img.mData = img.frameBuffer->buf;
		img.len  = img.frameBuffer->len;
	} else {
		frame2jpg(img.frameBuffer, kJpegQuality, reinterpret_cast<uint8_t **>(&img.mData), &img.len);
	}

	return imgPtr;
}
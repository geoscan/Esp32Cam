#include "Ov2640.hpp"
#include "esp_camera.h"
#include <algorithm>

using namespace std;

// ------------ Ov2640 ------------ //


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

	esp_camera_init(&cameraConfig);
}

std::shared_ptr<Cam::Frame> Ov2640::getFrame()
{
	auto fb = esp_camera_fb_get();

	if (!fb) {
		return std::make_shared<Cam::Frame>();
	}

	return std::shared_ptr<Cam::Frame>(new Ov2640::Frame(fb));
}


// ------------ Ov2640::Frame ------------ //

Ov2640::Frame::Frame(camera_fb_t *aFb) : Cam::Frame(aFb->buf, aFb->len), fb(aFb)
{
}

Ov2640::Frame::~Frame()
{
	esp_camera_fb_return(fb);
}
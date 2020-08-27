#include "Ov2640.hpp"
#include <algorithm>
#include "CameraLock.hpp"

using namespace std;

asio::const_buffer Ov2640::Image::data()
{
	return asio::buffer(mData, len);
}

bool Ov2640::Image::valid() const
{
	return frameBuffer != nullptr;
}

Ov2640::Image::~Image()
{
	CamOv2640::CameraLock lock;
	esp_camera_fb_return(frameBuffer);
	// TODO: WARNING: make a proper inquiry about whether we should free 'raw',
	// or 'esp_camera_fb_return' will do that for us.
}


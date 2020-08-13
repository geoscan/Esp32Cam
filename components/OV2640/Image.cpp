#include "Image.hpp"
#include <algorithm>

using namespace std;

Image::Image(Image &&i)
{
	swap(i.frameBuffer, frameBuffer);
	swap(i.raw, raw);
}

Image::Data Image::data() const
{
    return {raw, rawLen};
}

bool Image::isValid() const
{
	return valid;
}

Image &Image::operator=(Image &&i)
{
	swap(i.frameBuffer, frameBuffer);
	swap(i.raw, raw);

	return *this;
}

Image::~Image()
{
	esp_camera_fb_return(frameBuffer);
	// TODO: WARNING: make a proper inquiry about whether we should free 'raw',
	// or 'esp_camera_fb_return' will do that for us.
}


#include "Image.hpp"
#include <algorithm>

using namespace std;

namespace std {
	void swap(Image& l, Image& r)
	{
		swap(l.frameBuffer, r.frameBuffer);
		swap(l.data.data,   r.data.data);
		swap(l.data.len,    r.data.len);
		swap(l.valid,       r.valid);
	}
}

Image::Image(Image &&i)
{
	swap(*this, i);
}

const Image::Data &Image::getData() const
{
	return data;
}

bool Image::isValid() const
{
	return valid;
}

Image &Image::operator=(Image &&i)
{
	swap(*this, i);

	return *this;
}

Image::~Image()
{
	esp_camera_fb_return(frameBuffer);
	// TODO: WARNING: make a proper inquiry about whether we should free 'raw',
	// or 'esp_camera_fb_return' will do that for us.
}


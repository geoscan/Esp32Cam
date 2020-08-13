#ifndef COMPONENTS_OV2640_IMAGE_HPP
#define COMPONENTS_OV2640_IMAGE_HPP

#include <utility>
#include "esp_camera.h"
#include "img_converters.h"

 // ptr to mem. and its length

class Image {
	friend class Ov2640;
public:
	struct Data;

	~Image();

	Data data() const;
	bool isValid() const;

	Image(const Image &)            = delete;
	Image &operator=(const Image &) = delete;

	Image(Image &&);
	Image &operator=(Image &&);
private:
	Image() = default;

	size_t      rawLen;
	void*       raw          {nullptr};
	camera_fb_t *frameBuffer {nullptr};
	bool        valid        {false};
};

struct Image::Data {
	const void*  data;
	const size_t len;
};

#endif // IMAGE_HPP

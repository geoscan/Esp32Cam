#ifndef COMPONENTS_OV2640_IMAGE_HPP
#define COMPONENTS_OV2640_IMAGE_HPP

#include <utility>
#include "esp_camera.h"
#include "img_converters.h"

using ImgData = std::pair<const void *, size_t>; // ptr to mem. and its length

class Image {
	friend class Ov2640;
public:
	~Image();

	ImgData data() const;

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

#endif // IMAGE_HPP

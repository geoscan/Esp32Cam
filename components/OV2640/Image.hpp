#ifndef COMPONENTS_OV2640_IMAGE_HPP
#define COMPONENTS_OV2640_IMAGE_HPP

#include <utility>
#include "esp_camera.h"
#include "img_converters.h"

 // ptr to mem. and its length

class Image;

namespace std {
	void swap(Image&, Image&);
}

class Image {
	friend class Ov2640;
	friend void std::swap(Image&, Image&);
public:
	struct Data {
		void*  data {nullptr};
		size_t len  {0};
	};

	~Image();

	const Data &getData() const;
	bool isValid() const;

	Image(const Image &)            = delete;
	Image &operator=(const Image &) = delete;

	Image(Image &&);
	Image &operator=(Image &&);
private:
	Image() = default;

//	size_t      rawLen;
//	void*       raw          {nullptr};
	Data        data;
	camera_fb_t *frameBuffer {nullptr};
	bool        valid        {false};
};

#endif // IMAGE_HPP

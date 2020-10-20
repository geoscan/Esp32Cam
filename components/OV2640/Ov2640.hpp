#ifndef COMPONENTS_OV2640_OV2640_H
#define COMPONENTS_OV2640_OV2640_H

#include <utility>
#include "img_converters.h"
#include <cstdint>
#include <memory>
#include "asio.hpp"

// Wrapper around C API of OV2640
// Both Ov2640's and Ov2640::Image's methods are thread-safe

void ov2640Init();

// --------------------------- Ov2640 --------------------------- //

class Ov2640 {
public:
	struct Image;

	static Ov2640 &instance();

	Ov2640(const Ov2640 &) = delete;
	Ov2640(Ov2640 &&) = delete;
	Ov2640 &operator=(const Ov2640 &) = delete;
	Ov2640 &operator=(Ov2640 &&) = delete;
	~Ov2640();

	std::unique_ptr<Ov2640::Image> jpeg();

private:
	static void init();
	static bool isInit;
	Ov2640();
};


// ---------------------------  Image --------------------------- //


struct Ov2640::Image final {
	friend class Ov2640;
	const camera_fb_t *frameBuffer() const;

	asio::const_buffer data();
	bool valid() const;
	Image(const Image &) = delete;
	Image(Image &&) = delete;
	Image &operator=(const Image &) = delete;
	Image &operator=(Image &) = delete;
	~Image();
private:
	Image() = default;
	void        *mData        {nullptr};
	size_t      len           {0};
	camera_fb_t *mFrameBuffer {nullptr};
};


#endif // COMPONENTS_OV2640_OV2640_H

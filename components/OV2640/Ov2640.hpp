#ifndef COMPONENTS_OV2640_OV2640_H
#define COMPONENTS_OV2640_OV2640_H

#include <utility>
#include "img_converters.h"
#include <cstdint>
#include <memory>
#include "asio.hpp"
#include "cam/Camera.hpp"

// Wrapper around C API for OV2640

// --------------------------- Ov2640 --------------------------- //

class Ov2640 : public Cam::CameraBase {
public:
	void init() override;
	std::shared_ptr<Cam::Frame> getFrame() override;
private:
	class Frame : public Cam::Frame {
		camera_fb_t *fb;
	public:
		using Cam::Frame::Buffer;
		Frame(camera_fb_t *);
		~Frame();
	};
};


#endif // COMPONENTS_OV2640_OV2640_H

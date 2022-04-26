#ifndef COMPONENTS_OV2640_OV2640_H
#define COMPONENTS_OV2640_OV2640_H

#include <utility>
#include "img_converters.h"
#include <cstdint>
#include <memory>
#include "asio.hpp"
#include "cam/Camera.hpp"
#include "sub/Sys.hpp"

// Wrapper around C API for OV2640

// --------------------------- Ov2640 --------------------------- //

class Ov2640 : public Cam::CameraBase {
public:
	Ov2640();
	void init() override;
	std::shared_ptr<Cam::Frame> getFrame() override;
private:
	class Frame : public Cam::Frame {
		camera_fb_t *fb;
	public:
		Frame &operator=(const Frame &) = delete;
		Frame(const Frame &) = delete;

		Frame(Frame &&);
		Frame &operator=(Frame &&);

		Frame(camera_fb_t *);
		~Frame();

		int width() override;
		int height() override;
	};

public:
	typename Sub::Sys::Fld::ModuleGetField::Ret moduleGetField(typename Sub::Sys::Fld::ModuleGetField::Arg<0>);

private:
	struct {
		Sub::Sys::Fld::ModuleGetField moduleGetState;
	} key;

	struct {
		bool initialized = false;
		struct {
			int h = -1;
			int w = -1;
		} frame;
	} status;
};


#endif // COMPONENTS_OV2640_OV2640_H

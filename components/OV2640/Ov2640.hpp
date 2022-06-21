#ifndef COMPONENTS_OV2640_OV2640_H
#define COMPONENTS_OV2640_OV2640_H

#include <utility>
#include "img_converters.h"
#include <cstdint>
#include <memory>
#include "asio.hpp"
#include "cam/Camera.hpp"
#include "utility/mod/ModuleBase.hpp"

// Wrapper around C API for OV2640

// --------------------------- Ov2640 --------------------------- //

class Ov2640 : public Cam::CameraBase, public Utility::Mod::ModuleBase {
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

protected:
	virtual typename Utility::Mod::Fld::ModuleGetFieldMult::Ret getFieldValue(
		typename Utility::Mod::Fld::ModuleGetFieldMult::Arg<0>,
		typename Utility::Mod::Fld::ModuleGetFieldMult::Arg<1>) override;

	struct {
		bool initialized = false;
		struct {
			int h = -1;
			int w = -1;
		} frame;
	} status;
};


#endif // COMPONENTS_OV2640_OV2640_H

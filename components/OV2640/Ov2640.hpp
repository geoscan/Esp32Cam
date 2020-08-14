#ifndef COMPONENTS_OV2640_OV2640_H
#define COMPONENTS_OV2640_OV2640_H

#include <utility>
#include "img_converters.h"
#include <cstdint>
#include "Image.hpp"

//enum class JpegQuality : uint8_t {
//	Lowest  =   1,
//	Low     =  25,
//	Medium  =  50,
//	High    =  75,
//	Highest = 100,
//};

class Ov2640 {
public:
	static Ov2640 &instance();

	Ov2640(const Ov2640 &) = delete;
	Ov2640(Ov2640 &&) = delete;
	Ov2640 &operator=(const Ov2640 &) = delete;
	Ov2640 &operator=(Ov2640 &&) = delete;
	~Ov2640();

	Image jpeg();

private:
	static void init();
	static bool isInit;
	Ov2640();
};


#endif // COMPONENTS_OV2640_OV2640_H

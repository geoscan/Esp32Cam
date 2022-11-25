//
// Fs.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Fs.hpp"

namespace Ut {
namespace Fs {

std::size_t fileSize(FILE *fp)
{
	std::size_t prevPos = ftell(fp);
	fseek(fp, SEEK_SET, SEEK_END);
	std::size_t size = ftell(fp);
	fseek(fp, SEEK_SET, prevPos);

	return size;
}

}  // namespace Fs
}  // namespace Ut

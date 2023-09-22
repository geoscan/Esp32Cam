//
// Assert.hpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_SYSTEM_PORT_ESPIDF_SYSTEM_OS_ASSERT_HPP
#define COMPONENTS_SYSTEM_PORT_ESPIDF_SYSTEM_OS_ASSERT_HPP

#include <assert.h>

namespace Sys {

inline void panic()
{
	assert(false);
}

}  // Sys

#endif // COMPONENTS_SYSTEM_PORT_ESPIDF_SYSTEM_OS_ASSERT_HPP

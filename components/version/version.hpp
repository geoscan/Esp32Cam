//
// Stm32Sink.hpp
//
// Created on:  Oct 05, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_VERSION_VERSION_HPP_IN
#define COMPONENTS_VERSION_VERSION_HPP_IN

// <VERSION_MAJOR>.<VERSION_MINOR>.<VERSION_PATCH>-<VERSION_SUFFIX>

#define ESP32_FIRMWARE_VERSION "0.2.5"

#if !defined(ESP32_FIRMWARE_VERSION)
# define ESP32_FIRMWARE_VERSION "0.0.0"
#endif

#include <string>

void versionInit();
bool versionStmGet(std::string&);

#endif  // COMPONENTS_VERSION_VERSION_HPP_IN

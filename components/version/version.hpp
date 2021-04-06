//
// Stm32Sink.hpp
//
// Created on:  Oct 05, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_VERSION_VERSION_HPP_H
#define COMPONENTS_VERSION_VERSION_HPP_H

// <VERSION_MAJOR>.<VERSION_MINOR>.<VERSION_PATCH>-<VERSION_SUFFIX>

#if !defined(ESP32_FIRMWARE_VERSION)
# define ESP32_FIRMWARE_VERSION "0.0.0"
#endif

#include <string>

void versionInit();
std::string connectedSerialVersionGet();

#endif  // COMPONENTS_VERSION_VERSION_HPP_H

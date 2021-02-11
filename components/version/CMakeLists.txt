file(GLOB SOURCE_FILES
		"*.c"
		"*.cpp")


idf_component_register(
	SRCS
		${SOURCE_FILES}
	INCLUDE_DIRS
		"."
	PRIV_INCLUDE_DIRS
		private_include
	REQUIRES
		uart
		utility)


#  -----  Configure ESP32 version  -----  #
# 
# Extracts actual git tag, checks whether
# it matches #.#.# format, and configures
# version.hpp file

execute_process(
	COMMAND git describe
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE ESP32_FIRMWARE_VERSION)

string(REGEX MATCH "(^[0-9]+.[0-9]+.[0-9]+-*[0-9]*)" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")

if(NOT ${ESP32_FIRMWARE_VERSION} STREQUAL "")

	string(REPLACE "\n" "" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")
	string(REPLACE "\r" "" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")
	add_compile_definitions(ESP32_FIRMWARE_VERSION="${ESP32_FIRMWARE_VERSION}")
endif()
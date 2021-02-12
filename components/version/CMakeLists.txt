# Register component

idf_component_register(
	SRC_DIRS "."
	INCLUDE_DIRS
		"."
	PRIV_INCLUDE_DIRS
		private_include
	REQUIRES
		uart
		utility)

# Add version macro

execute_process(
	COMMAND git describe
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE ESP32_FIRMWARE_VERSION)

string(REGEX MATCH "(^[0-9]+.[0-9]+.[0-9]+-*[0-9]*)" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")

if(NOT ${ESP32_FIRMWARE_VERSION} STREQUAL "")
	string(REPLACE "\n" "" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")
	string(REPLACE "\r" "" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")
	target_compile_definitions(${COMPONENT_LIB} PUBLIC ESP32_FIRMWARE_VERSION="${ESP32_FIRMWARE_VERSION}")
endif()

# Print version message
# https://stackoverflow.com/questions/18968979/how-to-get-colorized-output-with-cmake
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColorReset "${Esc}[m")
  set(Green      "${Esc}[1;32m")  # Bold; Green
endif()

message("${Green} [version]: ${ESP32_FIRMWARE_VERSION}${ColorReset}")
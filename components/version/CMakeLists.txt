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


execute_process(
	COMMAND git describe --abbrev=0
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE ESP32_FIRMWARE_VERSION)

file(REMOVE version.hpp)

string(REPLACE "\n" "" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")
string(REPLACE "\r" "" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp @ONLY)
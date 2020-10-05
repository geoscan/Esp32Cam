idf_component_register(
	INCLUDE_DIRS
		".")


#  -----  Configure version  -----  #


execute_process(
	COMMAND git describe --abbrev=0
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE ESP32_FIRMWARE_VERSION)

file(REMOVE version.hpp)

string(REPLACE "\n" "" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")
string(REPLACE "\r" "" ESP32_FIRMWARE_VERSION "${ESP32_FIRMWARE_VERSION}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp @ONLY)
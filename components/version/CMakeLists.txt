idf_component_register(
	INCLUDE_DIRS
		".")

execute_process(
	COMMAND git describe
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE ESP32_FIRMWARE_VERSION)

file(REMOVE version.hpp)
configure_file(verison.hpp.in version.hpp)
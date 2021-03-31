file(GLOB SOURCE_FILES
		"*.c"
		"*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		.
		rr/rr-include
	PRIV_INCLUDE_DIRS
	REQUIRES
		asio
		pthread
		esp_common
		OV2640
		freertos
)
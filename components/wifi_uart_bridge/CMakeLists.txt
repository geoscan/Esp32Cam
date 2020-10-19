file(GLOB SOURCE_FILES
		"*.c"
		"*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS "."
	REQUIRES
		freertos
		uart
		asio
#		esp_timer
		esp_common
		pthread
		utility)
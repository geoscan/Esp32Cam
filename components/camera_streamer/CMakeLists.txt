file(GLOB SOURCE_FILES
		"*.c"
		"*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS "."
	REQUIRES
		freertos
		OV2640
		esp_common
		utility
		lwip
		esp_wifi)
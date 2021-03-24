file(GLOB SOURCE_FILES
	*.c
	*.cpp
	avilib/*.c
)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		include
		avilib
	REQUIRES
		esp_common
		utility
		sd_fat
		etl
)
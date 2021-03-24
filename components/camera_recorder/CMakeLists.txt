file(GLOB SOURCE_FILES
	*.c
	*.cpp
	avilib/*.c
)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		include
	PRIV_INCLUDE_DIRS
		avilib
		priv_include
	REQUIRES
		esp_common
		utility
		sd_fat
		etl
)
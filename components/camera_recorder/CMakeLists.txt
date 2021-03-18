file(GLOB SOURCE_FILES
	*.c
	*.cpp
	avilib/*.c
)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS "."
	PRIV_INCLUDE_DIRS avilib
	REQUIRES
		esp_common
		utility
		sd_fat
		etl
)
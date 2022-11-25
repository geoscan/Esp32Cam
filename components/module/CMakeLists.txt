file(GLOB SOURCE_FILES
		"module/*.c"
		"module/*.cpp"
		"module/Parameter/*.cpp"
		)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS .
	REQUIRES
		esp_common
		utility
)

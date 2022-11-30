file(GLOB SOURCE_FILES
		"module/*.c"
		"module/*.cpp"
		"module/Parameter/*.cpp"
		"module/Parameter/MemoryProvider/*.cpp"
		)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS .
	REQUIRES
		esp_common
		utility
		json
)

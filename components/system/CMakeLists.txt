file(GLOB SOURCE_FILES
		"system/*.c"
		"system/*.cpp"
		)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		"."
		"port/espidf"
	REQUIRES
		esp_common
)

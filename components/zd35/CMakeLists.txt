file(GLOB SOURCE_FILES
		"zd35/*.c"
		"zd35/*.cpp"
		)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS .
	REQUIRES
		esp_common
)

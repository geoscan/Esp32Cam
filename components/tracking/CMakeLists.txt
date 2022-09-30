file(GLOB SOURCE_FILES
		"tracking/*.c"
		"tracking/*.cpp"
		)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS .
	REQUIRES
		esp_common
		mosse
)

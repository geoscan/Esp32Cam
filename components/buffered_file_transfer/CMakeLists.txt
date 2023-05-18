file(GLOB SOURCE_FILES
		"buffered_file_transfer/*.c"
		"buffered_file_transfer/*.cpp"
		)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS .
	REQUIRES
		esp_common
)

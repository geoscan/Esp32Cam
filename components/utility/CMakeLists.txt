file(GLOB SOURCE_FILES
		"utility/*.c"
		"utility/*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS "."
	REQUIRES
		asio)
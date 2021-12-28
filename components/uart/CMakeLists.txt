file(GLOB SOURCE_FILES
		"*.c"
		"*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		"."
	PRIV_INCLUDE_DIRS
		priv_include
	REQUIRES
		freertos
		utility)

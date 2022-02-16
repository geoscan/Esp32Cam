file(GLOB SOURCE_FILES
		"*.c"
		"*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		"."
		"include"
	PRIV_INCLUDE_DIRS
		priv_include
	REQUIRES
		sub
		freertos
		utility)

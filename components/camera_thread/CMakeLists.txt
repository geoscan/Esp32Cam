file(GLOB SOURCE_FILES
		"*.c"
		"*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	PRIV_INCLUDE_DIRS
		priv_include
	INCLUDE_DIRS
		include
	REQUIRES
		sub
		utility
		cam
)
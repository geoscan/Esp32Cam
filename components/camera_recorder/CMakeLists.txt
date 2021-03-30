file(GLOB SOURCES *.c *.cpp)

idf_component_register(
	SRCS
		${SOURCES}
	INCLUDE_DIRS
		.
	REQUIRES
		utility
		avilib
)
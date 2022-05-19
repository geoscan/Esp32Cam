file(GLOB SOURCES *.c *.cpp)

idf_component_register(
	SRCS
		${SOURCES}
	INCLUDE_DIRS
		.
	REQUIRES
		sub
		utility
		avilib
		sd_fat
)

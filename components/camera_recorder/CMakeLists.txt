file(GLOB SOURCES *.c *.cpp)

idf_component_register(
	SRCS
		${SOURCES}
	INCLUDE_DIRS
		camera_recorder
	REQUIRES
		utility
)
file(GLOB SOURCES *.c *.cpp)

idf_component_register(
	SRCS ${SOURCES}
	PRIV_INCLUDE_DIRS private_include
	INCLUDE_DIRS log_udp
	REQUIRES
		asio
		esp_common
		utility
)
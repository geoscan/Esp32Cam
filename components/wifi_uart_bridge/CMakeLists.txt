file(GLOB SOURCE_FILES
	"wifi_uart_bridge/*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		.
	REQUIRES
		freertos
		uart
		asio
		esp_common
		pthread
		utility
		socket
		sub)

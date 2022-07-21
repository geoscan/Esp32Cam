file(GLOB SOURCE_FILES
		"*.c"
		"*.cpp"
		"wifi_uart_bridge/*.cpp")

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		.
		include
	PRIV_INCLUDE_DIRS
		priv_include
	REQUIRES
		freertos
		uart
		asio
		esp_common
		pthread
		utility
		socket
		sub)

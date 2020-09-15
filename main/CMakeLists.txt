#idf_component_register(SRCS "tcp_server.c"
#                    INCLUDE_DIRS ".")

idf_component_register(
	SRCS "main.cpp"
	REQUIRES
		wifi
		http
		rtsp
		echo_tcp
		wifi_uart_bridge
		esp_timer)

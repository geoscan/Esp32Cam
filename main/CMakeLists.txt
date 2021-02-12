#idf_component_register(SRCS "tcp_server.c"
#                    INCLUDE_DIRS ".")

idf_component_register(
	SRCS "main.cpp"
	REQUIRES
		wifi
		http
		rtsp
		wifi_uart_bridge
		deprecated_wifi_uart_bridge
		esp_common
		camera_streamer
		asio
		pthread
		OV2640
		version
		sd_example
		sd_fat)

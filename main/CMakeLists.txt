#idf_component_register(SRCS "tcp_server.c"
#                    INCLUDE_DIRS ".")

idf_component_register(
	SRCS "main.cpp"
	REQUIRES
		wifi
		http
		wifi_uart_bridge
		esp_common
		camera_streamer
		asio
		pthread
		OV2640
		version
		sd_fat
		esp_event
		log_udp
		camera_thread
		mav
		uart
		socket
		wq
)

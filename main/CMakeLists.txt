#idf_component_register(SRCS "tcp_server.c"
#                    INCLUDE_DIRS ".")

idf_component_register(
	SRCS "main.cpp"
	REQUIRES
		OV2640
		asio
		buffered_file_transfer
		camera_streamer
		camera_thread
		esp_common
		esp_event
		http
		log_udp
		mav
		module
		pthread
		sd_fat
		socket
		tracking
		uart
		wifi
		wifi_uart_bridge
		wq
)

file(GLOB SOURCE_FILES
		"buffered_file_transfer/*.c"
		"buffered_file_transfer/*.cpp"
		"buffered_file_transfer/storage/*.c"
		"buffered_file_transfer/storage/*.cpp"
		)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS .
	REQUIRES
		esp_common
	PRIV_REQUIRES
		wifi_uart_bridge
		wq
)

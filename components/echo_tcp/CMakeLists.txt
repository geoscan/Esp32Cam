idf_component_register(
	SRCS "echo_tcp.cpp"
	INCLUDE_DIRS "."
	REQUIRES
		asio
		protocol_examples_common
		nvs_flash)
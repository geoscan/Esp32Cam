file(GLOB
	SOURCE_FILES
		"*.c"
		"*.cpp"
		"pages/*.c"
		"pages/*.cpp")

idf_component_register(
	SRCS         ${SOURCE_FILES}
    INCLUDE_DIRS "."
    REQUIRES
        wifi
        OV2640
        protocol_examples_common
        esp_http_server)
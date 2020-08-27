file(GLOB
	SOURCE_FILES
		"*.c"
		"*.cpp")

idf_component_register(
	SRCS
		${SOURCE_FILES}
		request_parser/generated/lex.yy.cpp
		request_parser/generated/rtsp.tab.cpp
    INCLUDE_DIRS
		"."
#	PRIV_INCLUDE_DIRS
		"./request_parser/generated"
    REQUIRES
        asio
		pthread
		esp_common
		freertos
		OV2640)
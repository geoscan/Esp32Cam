#set(
#	COMPONENT_SRCS
#		driver/camera.c
#		driver/sccb.c
#		driver/sensor.c
#		driver/twi.c
#		driver/xclk.c
#		sensors/ov2640.c
#		sensors/ov3660.c
#		sensors/ov5640.c
#		sensors/ov7725.c
#		conversions/yuv.c
#		conversions/to_jpg.cpp
#		conversions/to_bmp.c
#		conversions/jpge.cpp
#		conversions/esp_jpg_decode.c
#		Ov2640.cpp
#		Image.cpp)

#set(
#	COMPONENT_ADD_INCLUDEDIRS
#		driver/include
#		conversions/include
#  )

#set(
#	COMPONENT_PRIV_INCLUDEDIRS
#		.
#		driver/private_include
#		sensors/private_include
#		conversions/private_include
#		)

#set(COMPONENT_REQUIRES driver)
#set(COMPONENT_PRIV_REQUIRES freertos nvs_flash)

#register_component()

#file(GLOB
#	SOURCE_FILES
#		"conversions/*.c"
#		"conversions/*.cpp"
#		"sensors/*.c"
#		"sensors/*.cpp"
#		"driver/*.c"
#		"driver/*.cpp"
#		"./*.c"
#		"./*.cpp")

idf_component_register(
	SRC_DIRS
		"."
		"driver"
		"conversions"
		"sensors"
	INCLUDE_DIRS
		"."
		"driver/include"
		"conversions/include"
	PRIV_INCLUDE_DIRS
		"driver/private_include"
		"sensors/private_include"
		"conversions/private_include"
	REQUIRES
		wifi
		OV2640
		protocol_examples_common
		esp_http_server)
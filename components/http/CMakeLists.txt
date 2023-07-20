file(GLOB
	SOURCE_FILES
		"*.c"
		"*.cpp"
		"client/*.c"
		"client/*.cpp"
		"http/*.c"
		"http/*.cpp"
		"pages/*.c"
		"pages/*.cpp"
)

idf_component_register(
	SRCS         ${SOURCE_FILES}
	INCLUDE_DIRS "."
	REQUIRES
	    cam
		camera_recorder
		esp_common
		esp_http_client
		esp_http_server
		json
		sd_fat
		utility
		version
		wifi
	PRIV_REQUIRES
		buffered_file_transfer
	EMBED_FILES
		"resources/upload_script.html"
)

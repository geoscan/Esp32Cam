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
		cam
		esp_common
		json
		esp_http_server
		version
		camera_recorder
		sd_fat
		utility
	EMBED_FILES
		"resources/upload_script.html"
)

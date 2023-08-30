file(GLOB SOURCE_FILES
		"port/espidf/system/os/*.cpp"
		"system/*.c"
		"system/*.cpp"
		"system/hardware/*.cpp"
		"system/middleware/*.cpp"
		"system/os/*.cpp"
		)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS
		"."
		"port/espidf"
	REQUIRES
		esp_common
)

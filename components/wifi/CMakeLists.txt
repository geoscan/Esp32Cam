file(GLOB SOURCE_FILES "*.cpp" "wifi/*.cpp")
idf_component_register(
	SRCS
		${SOURCE_FILES}
	INCLUDE_DIRS "."
	REQUIRES
		esp_wifi
		nvs_flash
		esp_event
		freertos
		esp_common
		log
		uart
		utility
)

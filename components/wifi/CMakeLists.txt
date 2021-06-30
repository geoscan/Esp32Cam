idf_component_register(
	SRCS
		"wifi.cpp"
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

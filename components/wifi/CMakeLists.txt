# Edit following two lines to set component requirements (see docs)
#set(COMPONENT_REQUIRES )
#set(COMPONENT_PRIV_REQUIRES esp_wifi)

#set(COMPONENT_SRCS "wifi.c")
#set(COMPONENT_ADD_INCLUDEDIRS "include")

#register_component()

idf_component_register(
	SRCS "wifi.c"
	INCLUDE_DIRS "."
	REQUIRES
		esp_wifi
		nvs_flash
		esp_event
		freertos
		esp_common
		log)
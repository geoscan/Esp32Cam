file(GLOB SOURCE_FILES
	"tracking/*.c"
	"tracking/*.cpp"
	"priv_include/*.cpp"
	)

idf_component_register(
	SRCS ${SOURCE_FILES}
	INCLUDE_DIRS .
	PRIV_INCLUDE_DIRS
		priv_include
	REQUIRES
		esp_common
		mosse
		sub
)

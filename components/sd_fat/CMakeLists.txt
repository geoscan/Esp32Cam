idf_component_register(
	SRC_DIRS "."
	REQUIRES
		sdmmc
		fatfs
		driver
	INCLUDE_DIRS "."
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC COMPONENT_NAME="[${COMPONENT_NAME}]")
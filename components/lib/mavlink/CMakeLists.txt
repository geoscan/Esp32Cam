set(PATH_MAVLINK_HEADERS ${PROJECT_BINARY_DIR}/lib)

if(NOT EXISTS ${PATH_MAVLINK_HEADERS})
	add_subdirectory(mavlink EXCLUDE_FROM_ALL)
	file(MAKE_DIRECTORY ${PATH_MAVLINK_HEADERS})
	add_custom_target(MavlinkCopyGenerated
		COMMENT "Copying generated MAVLink files"
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${mavlink_BINARY_DIR}/include ${PATH_MAVLINK_HEADERS}/mavlink
	)
	add_dependencies(MavlinkCopyGenerated geoscan.xml-v2.0)  # Ensure the correct order of execution

	idf_component_register(
		INCLUDE_DIRS
			${PATH_MAVLINK_HEADERS}
	)
	add_dependencies(${COMPONENT_LIB} MavlinkCopyGenerated)
else()
	idf_component_register(
		INCLUDE_DIRS
			${PATH_MAVLINK_HEADERS})
endif()

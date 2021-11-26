idf_component_register()
set(PATH_MAVLINK_HEADERS ${PROJECT_BINARY_DIR}/mavlink/generated)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mavlink EXCLUDE_FROM_ALL)
add_custom_target(MavlinkCopyGenerated
	COMMENT "Copying generated MAVLink files"
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${mavlink_BINARY_DIR}/include ${PATH_MAVLINK_HEADERS}/mavlink)
add_dependencies(MavlinkCopyGenerated geoscan.xml-v2.0)  # Ensure the correct order of execution

# To ensure that unnecessary recompiling caused by updated headers is not triggered
if (NOT EXISTS ${PATH_MAVLINK_HEADERS})
	add_dependencies(${COMPONENT_LIB} MavlinkCopyGenerated)
endif()

target_include_directories(${COMPONENT_LIB} INTERFACE ${PATH_MAVLINK_HEADERS})

# Register component

idf_component_register()

add_subdirectory(libetl)
target_link_libraries(${COMPONENT_LIB} INTERFACE libetl)
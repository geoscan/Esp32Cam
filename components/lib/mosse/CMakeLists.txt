idf_component_register(
	PRIV_INCLUDE_DIRS
	INCLUDE_DIRS "."
	SRC_DIRS "."
)

set(MOSSE_PORTABLE ON CACHE BOOL "")
set(MOSSE_PORT_USE_CASSERT ON CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/mosse EXCLUDE_FROM_ALL)
target_link_libraries(${COMPONENT_LIB} PUBLIC embmosse)

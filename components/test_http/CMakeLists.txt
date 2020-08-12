idf_component_register(
    SRCS "http.c"
    INCLUDE_DIRS "."
    REQUIRES
        wifi
        camera_OVxxxx
        protocol_examples_common
        esp_http_server)
idf_component_register(
    SRCS "http.c"
    INCLUDE_DIRS "."
    REQUIRES
        wifi
        OV2640
        protocol_examples_common
        esp_http_server)
idf_component_register(
    SRCS "webserver.c"
    INCLUDE_DIRS "interface"
    PRIV_REQUIRES wifi_controller main
    REQUIRES bluetooth
)

idf_component_register(
    SRCS "free80211.c"
    INCLUDE_DIRS "include"
)
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
idf_component_register(
    SRCS
        "src/connection.c"
        "src/gateway_telemetry.c"
        "src/ota.c"

        "src/http/http.c"

    PRIV_INCLUDE_DIRS
        "src"

    INCLUDE_DIRS
        "include"

    REQUIRES
        esp_http_client
        json
        mqtt
)

target_compile_options(${COMPONENT_LIB} PUBLIC -std=gnu23)

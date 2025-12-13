idf_component_register(
    SRCS
        "src/encryption.c"
        "src/peer.c"
        "src/storage.c"
        "src/wmesh.c"

    PRIV_INCLUDE_DIRS
        "src"

    INCLUDE_DIRS
        "include"

    REQUIRES
        esp_wifi
        nvs_flash
        mbedtls
)

target_compile_options(${COMPONENT_LIB} PUBLIC -std=gnu23)

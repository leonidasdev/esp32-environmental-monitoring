idf_component_register(
    SRCS
        "project.c"

        "config/persistence.c"
        "config/provisioning.c"

        "nodes/common.c"
        "nodes/gateway.c"
        "nodes/node.c"

        "ota/http.c"
        "ota/ota.c"

        "sensors/sensors.c"
        "sensors/ulp.c"

        "services/gateway.c"
        "services/ota.c"
        "services/telemetry.c"

        "time/clock.c"

        "webserver/request.c"
        "webserver/webserver.c"
        "webserver/fs/file.c"
        "webserver/fs/persistence.c"

        "wifi/config.c"
        "wifi/scan.c"
        "wifi/sta_connect.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        thingsboard
        wmesh

        app_update
        esp_http_server
        esp_https_ota
        fatfs
        json
        lwip
        ulp
)

ulp_embed_binary(
    "ulp_sensors"
    "sensors/ulp.S"
    "ulp_c_source_file.c"
)

fatfs_create_spiflash_image(storage "../fs" FLASH_IN_PROJECT)

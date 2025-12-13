#include "services/telemetry.h"

#include <string.h>
#include "esp_log.h"

static const char *TAG = "SPV Mesh Telemetry";


esp_err_t spv_telemetry_send_msg(
    wmesh_handle_t *handle,
    const spv_telemetry_msg *msg,
    const wmesh_address_t gateway_address
) {
    size_t size = sizeof(uint8_t) + msg->num_datos * sizeof(*msg->datos) + sizeof(*msg) ;
    uint8_t *buffer = malloc(size);
    buffer[0] = TELEMETRY_TYPE_MSG;
    memcpy(&buffer[1], msg, msg->num_datos * sizeof(*msg->datos) + sizeof(*msg));
    esp_err_t err = wmesh_send(
        handle,gateway_address,
        CONFIG_SPV_TELEMETRY_SERVICE_ID,
        buffer,size
    );
    free(buffer);

    return err;
}


spv_telemetry_received_message_t spv_telemetry_decode_message(
    uint8_t *data,
    size_t data_size
) {
    spv_telemetry_received_message_t msg = {
        .type = TELEMETRY_TYPE_ERROR
    };

    if(data_size < 2){
        return msg;
    }

    spv_telemetry_message_type_t msg_type = data[0];
    void *payload = &data[1];
    size_t payload_size = data_size - 1;

    switch (msg_type)
    {
    case TELEMETRY_TYPE_MSG:
        if(payload_size >= sizeof(spv_telemetry_msg)){
            msg.msg = payload;
            if(payload_size - sizeof(spv_telemetry_msg) == msg.msg->num_datos * sizeof(*msg.msg->datos)){
                msg.type = TELEMETRY_TYPE_MSG;
                return msg;
            }
            return msg;
        }
        else{
            ESP_LOGE(TAG,
					"Wrong length for advertisement. Expected %zu, but got %zu",
					sizeof(*msg.msg), payload_size
				);
            return msg;
            }

    default:
        return msg;
    }
}

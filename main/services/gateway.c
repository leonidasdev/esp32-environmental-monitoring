#include "services/gateway.h"

#include <string.h>
#include "esp_log.h"

static const char *TAG = "SPV Mesh Gateway";


esp_err_t spv_gateway_send_channel(
	wmesh_handle_t *handle,
	const spv_gateway_channel_advertisement_t *advertisement
) {
	uint8_t buffer[sizeof(uint8_t) + sizeof(*advertisement)];

	buffer[0] = GATEWAY_TYPE_CHANNEL;
	memcpy(buffer + 1, advertisement, sizeof(*advertisement));

	return wmesh_send(
		handle, wmesh_broadcast_address,
		CONFIG_SPV_GATEWAY_SERVICE_ID,
		buffer, sizeof(buffer)
	);
}


esp_err_t spv_gateway_send_advertisement(
	wmesh_handle_t *handle,
	const spv_gateway_advertisement_t *advertisement
) {
	uint8_t buffer[sizeof(uint8_t) + sizeof(*advertisement)];

	buffer[0] = GATEWAY_TYPE_ADVERTISEMENT;
	memcpy(buffer + 1, advertisement, sizeof(*advertisement));

	return wmesh_send(
		handle, wmesh_broadcast_address,
		CONFIG_SPV_GATEWAY_SERVICE_ID,
		buffer, sizeof(buffer)
	);
}


esp_err_t spv_gateway_send_sleep(
	wmesh_handle_t *handle,
	const spv_gateway_sleep_command_t *sleep_command
) {
	uint8_t buffer[sizeof(uint8_t) + sizeof(*sleep_command)];

	buffer[0] = GATEWAY_TYPE_SLEEP;
	memcpy(buffer + 1, sleep_command, sizeof(*sleep_command));

	return wmesh_send(
		handle, wmesh_broadcast_address,
		CONFIG_SPV_GATEWAY_SERVICE_ID,
		buffer, sizeof(buffer)
	);
}


spv_gateway_received_message_t spv_gateway_decode_message(
	uint8_t *data,
	size_t data_size
) {
	spv_gateway_received_message_t message = {
		.type = GATEWAY_TYPE_ERROR
	};

	// At least message type and data.
	if(data_size < 2) {
		return message;
	}

	spv_gateway_message_type_t message_type = data[0];
	void *payload = &data[1];
	size_t payload_size = data_size - 1;

	switch(message_type) {
		case GATEWAY_TYPE_CHANNEL:
			if(payload_size != sizeof(*message.channel)) {
				ESP_LOGE(TAG,
					"Wrong length for channel advertisement. Expected %zu, but got %zu",
					sizeof(*message.channel), payload_size
				);
				return message;
			}

			message.channel = payload;
			message.type = GATEWAY_TYPE_CHANNEL;
			return message;

		case GATEWAY_TYPE_ADVERTISEMENT:
			if(payload_size != sizeof(*message.advertisement)) {
				ESP_LOGE(TAG,
					"Wrong length for advertisement. Expected %zu, but got %zu",
					sizeof(*message.advertisement), payload_size
				);
				return message;
			}

			message.advertisement = payload;
			message.type = GATEWAY_TYPE_ADVERTISEMENT;
			return message;

		case GATEWAY_TYPE_SLEEP:
			if(payload_size != sizeof(*message.sleep_command)) {
				ESP_LOGE(TAG,
					"Wrong length for sleep command. Expected %zu, but got %zu",
					sizeof(*message.sleep_command), payload_size
				);
				return message;
			}

			message.sleep_command = payload;
			message.type = GATEWAY_TYPE_SLEEP;
			return message;

		default:

			return message;
	}
}

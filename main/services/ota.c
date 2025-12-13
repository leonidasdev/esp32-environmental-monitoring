#include "services/ota.h"

#include <string.h>
#include "esp_log.h"

static const char *TAG = "SPV Mesh OTA";


static esp_err_t send_mesh(
	wmesh_handle_t *handle,
	const wmesh_address_t dst,
	spv_ota_message_type_t message_type,
	const uint8_t *data,
	size_t data_size
);


esp_err_t spv_ota_send_request(
	wmesh_handle_t *handle,
	const wmesh_address_t gateway_address
) {
	return send_mesh(
		handle,
		gateway_address,
		OTA_TYPE_REQUEST,
		(uint8_t*) &(spv_ota_request_t) {0},
		sizeof(spv_ota_request_t)
	);
}


esp_err_t spv_ota_send_begin(
	wmesh_handle_t *handle,
	const spv_ota_begin_t *message
) {
	return send_mesh(
		handle,
		wmesh_broadcast_address,
		OTA_TYPE_BEGIN,
		(uint8_t*) message,
		sizeof(*message)
	);
}


esp_err_t spv_ota_send_data(
	wmesh_handle_t *handle,
	const spv_ota_data_t *data
) {
	return send_mesh(
		handle,
		wmesh_broadcast_address,
		OTA_TYPE_DATA,
		(uint8_t*) data,
		sizeof(*data)
	);
}


esp_err_t spv_ota_send_retry_request(
	wmesh_handle_t *handle,
	const wmesh_address_t gateway_address,
	const spv_ota_retry_request_t *data
) {
	return send_mesh(
		handle,
		wmesh_broadcast_address,
		OTA_TYPE_RETRY,
		(uint8_t*) data,
		sizeof(*data)
	);
}


esp_err_t spv_ota_send_end(
	wmesh_handle_t *handle
) {
	return send_mesh(
		handle,
		wmesh_broadcast_address,
		OTA_TYPE_END,
		(uint8_t*) &(spv_ota_end_t) {0},
		sizeof(spv_ota_end_t)
	);
}


spv_ota_received_message_t spv_ota_decode_message(
	uint8_t *data,
	size_t data_size
) {

	spv_ota_received_message_t msg = {
		.type = OTA_TYPE_ERROR
	};

	if(data_size < 1) {
		return msg;
	}

	uint8_t id = data[0];
	if(id <= OTA_TYPE_FIRST || id >= OTA_TYPE_LAST) {
		return msg;
	}

	spv_ota_message_type_t type = (spv_ota_message_type_t) id;
	size_t payload_size = data_size - 1;
	void *payload = &data[1];

	switch(type) {
		case OTA_TYPE_REQUEST:
			if(payload_size != sizeof(*msg.request)) {
				return msg;
			}
			msg.type = type;
			msg.request = payload;
			return msg;

		case OTA_TYPE_BEGIN:
			if(payload_size != sizeof(*msg.begin)) {
				return msg;
			}
			msg.type = type;
			msg.begin = payload;
			return msg;

		case OTA_TYPE_DATA:
			if(payload_size != sizeof(*msg.data)) {
				return msg;
			}
			msg.type = type;
			msg.data = payload;
			return msg;

		case OTA_TYPE_RETRY:
			if(payload_size != sizeof(*msg.retry)) {
				return msg;
			}
			msg.type = type;
			msg.retry = payload;
			return msg;

		case OTA_TYPE_END:
			if(payload_size != sizeof(*msg.end)) {
				return msg;
			}
			msg.type = type;
			msg.end = payload;
			return msg;

		default:
			return msg;
	}
}


static esp_err_t send_mesh(
	wmesh_handle_t *handle,
	const wmesh_address_t dst,
	spv_ota_message_type_t message_type,
	const uint8_t *data,
	size_t data_size
) {
	esp_err_t err;


	uint8_t *buffer = malloc(data_size + 1);
	assert(buffer);
	buffer[0] = message_type;
	memcpy(&buffer[1], data, data_size);

	err = wmesh_send(
		handle,
		dst,
		CONFIG_SPV_OTA_SERVICE_ID,
		buffer,
		data_size + 1
	);
	free(buffer);

	return err;
}

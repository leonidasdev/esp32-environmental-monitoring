#include "nodes/node.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "ulp_common.h"

#include "ota/ota.h"
#include "sensors/sensors.h"
#include "sensors/ulp.h"
#include "services/gateway.h"
#include "services/ota.h"
#include "services/telemetry.h"
#include "wifi/wifi.h"
#include "wmesh/wmesh.h"


static const char *TAG = "SPV node";
static esp_err_t gateway_cb(wmesh_handle_t *handle, wmesh_address_t src, uint8_t *data, size_t data_size, void *user_ctx);
static esp_err_t ota_cb(wmesh_handle_t *handle, wmesh_address_t src, uint8_t *data, size_t data_size, void *user_ctx);


typedef enum {
	NODE_STATE_WAIT_CHANNEL,
	NODE_STATE_WAIT_GATEWAY,
	NODE_STATE_SEND_TELEMETRY,
	NODE_STATE_WAIT_OTA_OR_SLEEP,
	NODE_STATE_OTA,
	NODE_STATE_WAIT_SLEEP,
	NODE_STATE_SLEEP
} node_state_t;

typedef struct {
	node_state_t state;
	wmesh_address_t gateway_address;
	spv_timestamp_t sleep_until;

	bool ota_requested;
	esp_ota_handle_t ota_handle;
	size_t ota_bytes;
	size_t ota_chunks;
	size_t ota_next_chunk;
} node_status_t;
static node_status_t node_status = {
	.state = NODE_STATE_WAIT_CHANNEL
};

static wmesh_service_config_t gateway_service_config = {
	.id = CONFIG_SPV_GATEWAY_SERVICE_ID,
	.receive_callback = gateway_cb,
	.ctx = &node_status
};

static wmesh_service_config_t ota_service_config = {
	.id = CONFIG_SPV_OTA_SERVICE_ID,
	.receive_callback = ota_cb,
	.ctx = &node_status,
};


void spv_start_node(
	const spv_config_t *config
) {

	ESP_LOGI(TAG, "Starting node");
	if(config->role != SPV_CONFIG_ROLE_NODE) {
		ESP_LOGE(TAG, "Device is not configured as node");
		abort();
	}

	ulp_timer_stop();
	spv_sensors_init();
	ESP_ERROR_CHECK(spv_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(spv_wifi_start());
	spv_wifi_set_channel(1);


	wmesh_config_t mesh_config = { 0 };
	memcpy(mesh_config.network_key, config->mesh_key, sizeof(mesh_config.network_key));
	wmesh_handle_t *handle = wmesh_init(&mesh_config);
	if(!handle) {
		ESP_LOGE(TAG, "Error initializing mesh");
		abort();
	}

	ESP_ERROR_CHECK(wmesh_register_service(
		handle,
		&gateway_service_config
	));
	ESP_ERROR_CHECK(wmesh_register_service(
		handle,
		&ota_service_config
	));


	while(node_status.state < NODE_STATE_SEND_TELEMETRY) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	if(esp_reset_reason() != ESP_RST_DEEPSLEEP) {
		node_status.state = node_status.ota_requested ?
			NODE_STATE_WAIT_OTA_OR_SLEEP : NODE_STATE_WAIT_SLEEP;

	} else if(node_status.state == NODE_STATE_SEND_TELEMETRY) {
		ESP_LOGI(TAG, "Sending telemetry");
		spv_telemetry_msg *msg = alloca(
			sizeof(*msg) + sizeof(*msg->datos) * CONFIG_SPV_TELEMETRY_CHUNK_SIZE
		);
		memcpy(msg->node_name, config->name, sizeof(msg->node_name));
		spv_timestamp_t reading_start_time = spv_ulp_start_time_get();

		ESP_LOGI(TAG,
			"Found readings from %"PRIu64 " (%"PRIu64 " seconds ago)",
			reading_start_time, time_get() - reading_start_time
		);

		size_t reading_count = spv_ulp_get_reading_count(SPV_ULP_NOISE_READING);
		for(size_t reading = 0; reading < reading_count; reading += CONFIG_SPV_TELEMETRY_CHUNK_SIZE) {
			ESP_LOGI(TAG, "Next chunk");
			msg->fecha = reading_start_time + reading * CONFIG_SPV_SENSOR_READ_FREQUENCY_SECONDS;

			size_t i = 0;
			for(; i + reading < reading_count && i < CONFIG_SPV_TELEMETRY_CHUNK_SIZE; i++) {
				ESP_LOGI(TAG, "Reading data %zu", i + reading);
				msg->datos[i].noise = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_NOISE_READING);
				msg->datos[i].luminosity = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_LUMINOSITY_READING);
				msg->datos[i].co2 = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_CO2_READING);
				msg->datos[i].voc = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_VOC_READING);
				msg->datos[i].humidity = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_HUMIDITY_READING);
				msg->datos[i].temperature = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_TEMPERATURE_READING);
			}
			msg->num_datos = i;

			spv_telemetry_send_msg(handle, msg, node_status.gateway_address);
		}

		node_status.state = node_status.ota_requested ?
			NODE_STATE_WAIT_OTA_OR_SLEEP : NODE_STATE_WAIT_SLEEP;
	}

	ESP_LOGI(TAG, "Waiting for sleep message");
	while(node_status.state <= NODE_STATE_WAIT_SLEEP) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	wmesh_stop(handle);
	nvs_flash_deinit();

	spv_ulp_start();
	spv_timestamp_t sleep_seconds = node_status.sleep_until - time_get();
	sleep_seconds = sleep_seconds > 60 * CONFIG_SPV_WAKE_INTERVAL_MINUTES ?
		60 * CONFIG_SPV_WAKE_INTERVAL_MINUTES : sleep_seconds;
	ESP_LOGI(TAG, "Sleeping for %"PRIu64 " seconds", sleep_seconds);
	esp_deep_sleep(sleep_seconds * 1000 * 1000);
}


static esp_err_t gateway_cb(wmesh_handle_t *handle, wmesh_address_t src, uint8_t *data, size_t data_size, void *user_ctx) {
	spv_gateway_received_message_t message = spv_gateway_decode_message(data, data_size);
	node_status_t *node_status = user_ctx;

	switch(message.type) {
		case GATEWAY_TYPE_CHANNEL:
			if(node_status->state != NODE_STATE_WAIT_CHANNEL) {
				ESP_LOGW(TAG, "Received channel announcement. Ignoring");
				return ESP_OK;
			}

			ESP_LOGI(TAG,
				"Found gateway on channel %"PRIu8,
				message.channel->channel
			);
			spv_wifi_set_channel(message.channel->channel);
			node_status->state = NODE_STATE_WAIT_GATEWAY;
			return ESP_OK;


		case GATEWAY_TYPE_ADVERTISEMENT:
			if(node_status->state != NODE_STATE_WAIT_GATEWAY) {
				ESP_LOGI(TAG, "Received reduntant gateway advertisement. Ignoring");
				return ESP_OK;
			}

			time_set(message.advertisement->current_time);
			ESP_LOGI(TAG,
				"Received gateway advertisement from %02X:%02X:%02X:%02X:%02X:%02X",
				src[0], src[1], src[2], src[3], src[4], src[5]
			);

			ESP_LOGI(TAG, "	Current time:		%"PRIu64, message.advertisement->current_time);
			ESP_LOGI(TAG, "	Firmware version:	%"PRIu64, message.advertisement->firmware_version);
			ESP_LOGI(TAG, "	Connectivity:		%s", message.advertisement->flags.has_connectivity ? "yes" : "no");
			memcpy(node_status->gateway_address, src, sizeof(node_status->gateway_address));

			if(message.advertisement->firmware_version > spv_ota_get_current_version()) {
				ESP_LOGI(TAG, "Gateway has a newer firmware, requesting OTA");
				spv_ota_send_request(handle, node_status->gateway_address);
				node_status->ota_requested = true;
			}

			if(!message.advertisement->flags.has_connectivity) {
				ESP_LOGW(TAG, "Gateway has no connectivity, waiting for sleep");
				node_status->state = node_status->ota_requested ?
					NODE_STATE_WAIT_OTA_OR_SLEEP : NODE_STATE_WAIT_SLEEP;
			} else {
				node_status->state = NODE_STATE_SEND_TELEMETRY;
			}

			return ESP_OK;


		case GATEWAY_TYPE_SLEEP:
			if(node_status->state != NODE_STATE_WAIT_SLEEP && node_status->state != NODE_STATE_WAIT_OTA_OR_SLEEP) {
				ESP_LOGW(TAG, "Received sleep command. Ignoring");
				return ESP_OK;
			}

			node_status->state = NODE_STATE_SLEEP;
			node_status->sleep_until = message.sleep_command->wake_time;
			return ESP_OK;

		default:
			ESP_LOGE(TAG, "Received gateway error");
			return ESP_FAIL;
	}
}


static esp_err_t ota_cb(wmesh_handle_t *handle, wmesh_address_t src, uint8_t *data, size_t data_size, void *user_ctx) {
	spv_ota_received_message_t msg = spv_ota_decode_message(data, data_size);
	node_status_t *node_status = user_ctx;
	esp_err_t err;


	size_t chunk_size = CONFIG_SPV_OTA_SERVICE_CHUNK_SIZE;
	switch (msg.type) {
		case OTA_TYPE_BEGIN:
			if(!node_status->ota_requested) {
				return ESP_OK;
			}

			if(node_status->state != NODE_STATE_WAIT_OTA_OR_SLEEP) {
				return ESP_OK;
			}

			if(node_status->state == NODE_STATE_OTA) {
				ESP_LOGW(TAG, "Received OTA start, but OTA is already in progress");
				return ESP_OK;
			}

			size_t ota_bytes = msg.begin->size;
			size_t chunk_count = (msg.begin->size + chunk_size - 1) / chunk_size;

			ESP_LOGI(TAG, "Starting OTA:");
			ESP_LOGI(TAG, "	Version:	%"PRIu64, msg.begin->version);
			ESP_LOGI(TAG, "	Size:		%zu Bytes", msg.begin->size);
			ESP_LOGI(TAG, "	Chunks:		%zu" , chunk_count);


			if((err = spv_ota_begin(ota_bytes, &node_status->ota_handle)) != ESP_OK) {
				ESP_LOGE(TAG, "Error starting OTA: %s", esp_err_to_name(err));
				node_status->state = NODE_STATE_WAIT_SLEEP;
			}
			node_status->ota_chunks = chunk_count;
			node_status->ota_bytes = ota_bytes;
			node_status->state = NODE_STATE_OTA;
			return ESP_OK;

		case OTA_TYPE_DATA:
			if(node_status->state != NODE_STATE_OTA) {
				return ESP_OK;
			}

			size_t expected_chunk = node_status->ota_next_chunk;
			size_t received_chunk = msg.data->chunk;
			if(expected_chunk > received_chunk) {
				// Another node has restarted the OTA update.
				return ESP_OK;
			}

			if(expected_chunk < received_chunk) {
				ESP_LOGW(TAG,
					"Expected OTA chunk [%zu], but received [%zu]"
					", requesting retry",
					expected_chunk, received_chunk
				);
				spv_ota_send_retry_request(
					handle, node_status->gateway_address,
					&(spv_ota_retry_request_t) {
						.restart_from = expected_chunk
					}
				);
				return ESP_OK;
			}

			if(received_chunk % 100 == 0) {
				ESP_LOGI(TAG,
					"Received chunk [%zu/%zu]",
					received_chunk, node_status->ota_chunks - 1
				);
			}

			size_t recv_byte_offset = received_chunk * chunk_size;
			size_t data_size = received_chunk == node_status->ota_chunks - 1 ?
				node_status->ota_bytes - recv_byte_offset :
				CONFIG_SPV_OTA_SERVICE_CHUNK_SIZE;

			err = esp_ota_write(node_status->ota_handle, msg.data->data, data_size);

			if(err != ESP_OK) {
				ESP_LOGE(TAG, "Error writing OTA chunk %s", esp_err_to_name(err));
				ESP_LOGE(TAG, "Aborting OTA");
				spv_ota_abort(node_status->ota_handle);
				node_status->state = NODE_STATE_WAIT_SLEEP;
			} else {
				node_status->ota_next_chunk++;
			}

			return ESP_OK;


		case OTA_TYPE_END:
			if(node_status->state != NODE_STATE_OTA) {
				return ESP_OK;
			}

			ESP_LOGI(TAG, "OTA finished");
			err = spv_ota_end(node_status->ota_handle);
			if(err != ESP_OK) {
				ESP_LOGE(TAG, "Error committing OTA: %s", esp_err_to_name(err));
			}

			node_status->state = NODE_STATE_WAIT_SLEEP;
			return ESP_OK;

		default:
			return ESP_OK;
	}

	return ESP_OK;
}

#include "nodes/gateway.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_netif_sntp.h"
#include "esp_image_format.h"
#include "ulp_common.h"

#include "nodes/common.h"
#include "thingsboard/thingsboard.h"
#include "ota/ota.h"
#include "sensors/sensors.h"
#include "sensors/ulp.h"
#include "services/gateway.h"
#include "services/ota.h"
#include "services/telemetry.h"
#include "wifi/wifi.h"
#include "wmesh/wmesh.h"

static const char *TAG = "SPV gateway";

static esp_err_t telemetry_cb(wmesh_handle_t *handle, wmesh_address_t src, uint8_t *data, size_t data_size, void *user_ctx);
static esp_err_t ota_cb(wmesh_handle_t *handle, wmesh_address_t src, uint8_t *data, size_t data_size, void *user_ctx);

typedef struct {
	bool connectivity;
	thingsboard_handle_t *thingsboard_handle;
	spv_timestamp_t last_telemetry;

	bool ota_requested;
	size_t next_chunk;
	bool retry_requested;
	size_t chunks_since_retry;
} gateway_status_t;
static gateway_status_t gateway_status = {
	.ota_requested = false,
	0
};

static void gateway_perform_ota(wmesh_handle_t *handle, gateway_status_t *gateway_status);

static wmesh_service_config_t telemetry_service_config = {
	.id = CONFIG_SPV_TELEMETRY_SERVICE_ID,
	.receive_callback = telemetry_cb,
	.ctx = &gateway_status,
};

static wmesh_service_config_t ota_service_config = {
	.id = CONFIG_SPV_OTA_SERVICE_ID,
	.receive_callback = ota_cb,
	.ctx = &gateway_status,
};


RTC_DATA_ATTR uint8_t boots_since_ntp_sync;


void spv_start_gateway(
	const spv_config_t *config
) {

	esp_err_t err;
	ESP_LOGI(TAG, "Starting gateway");
	if(config->role != SPV_CONFIG_ROLE_GATEWAY) {
		ESP_LOGE(TAG, "Device is not configured as gateway");
		abort();
	}

	ulp_timer_stop();
	spv_sensors_init();

	ESP_ERROR_CHECK(spv_wifi_set_mode(WIFI_MODE_APSTA));
	ESP_ERROR_CHECK(spv_wifi_start());
	spv_wifi_set_channel(1);
	wmesh_config_t mesh_config = { 0 };
	memcpy(mesh_config.network_key, config->mesh_key, sizeof(mesh_config.network_key));
	wmesh_handle_t *handle = wmesh_init(&mesh_config);

	if(!handle) {
		ESP_LOGE(TAG, "Error initializing mesh");
		abort();
	}

	spv_wifi_scan_results_t scan = { 0 };
	ESP_ERROR_CHECK(spv_wifi_scan(
		&scan,
		&(spv_wifi_scan_options_t) {
			.ssid_filter = (const char*[]) {
				config->gateway_config.wifi_ssid,
				0
			},
		}
	));

	bool connectivity = true;
	if(scan.network_count == 0) {
		ESP_LOGE(TAG, "No Wi-Fi networks found");
		spv_gateway_send_channel(
			handle,
			&(spv_gateway_channel_advertisement_t){
				.channel = 1
			}
		);

		connectivity = false;

	} else {

		ESP_LOGI(TAG, "Advertising on channel %"PRIu8, scan.networks[0].channel);
		spv_gateway_send_channel(
			handle,
			&(spv_gateway_channel_advertisement_t){
				.channel = scan.networks[0].channel
			}
		);

		esp_err_t err = spv_wifi_start_sta_connect(
			&(spv_wifi_sta_options_t) {
				.ssid = scan.networks[0].ssid,
				.bssid = scan.networks[0].bssid,
				.password = config->gateway_config.wifi_password,
				.retries = CONFIG_SPV_GATEWAY_SERVICE_WIFI_RETRIES
			}
		);

		if(err != ESP_OK) {
			ESP_LOGE(TAG, "Error connecting to Wi-Fi network");
			connectivity = false;
		}
	}


	thingsboard_handle_t thingsboard_handle = { 0 };
	if(connectivity) {
		ESP_LOGI(TAG, "Connecting to Thingsboard");
		thingsboard_connect_err_t ts_conn_err = thingsboard_connect(
			&(thingsboard_config_t) {
				.api_key = config->gateway_config.mqtt_key,
				.is_gateway = true
			},
			&thingsboard_handle
		);

		if(ts_conn_err != THINGSBOARD_CONNECT_OK) {
			ESP_LOGE(TAG, "Error connecting to Thingsboard");
			connectivity = false;
		}
	}

	if(connectivity && (esp_reset_reason() != ESP_RST_DEEPSLEEP || boots_since_ntp_sync > 5)) {
		ESP_LOGI(TAG, "Connecting to NTP server");
		esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
		esp_netif_sntp_init(&config);

		if(esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
	    	ESP_LOGE(TAG, "Failed to reach NTP server");
			connectivity = false;
		} else {
			ESP_LOGI(TAG, "System time updated: %"PRIu64, time_get());
		}
	} else {
		if(boots_since_ntp_sync < 255) {
			boots_since_ntp_sync++;
		}
	}


	if(connectivity && esp_reset_reason() == ESP_RST_DEEPSLEEP) {
		ESP_LOGI(TAG, "Sending telemetry");
		spv_telemetry_reading_t readings;
		spv_timestamp_t reading_start_time = spv_ulp_start_time_get();

		ESP_LOGI(TAG,
			"Found readings from %"PRIu64 " (%"PRIu64 " seconds ago)",
			reading_start_time, time_get() - reading_start_time
		);

		size_t reading_count = spv_ulp_get_reading_count(SPV_ULP_NOISE_READING)*0 + 3;

		for(size_t reading = 0; reading < reading_count; reading += CONFIG_SPV_TELEMETRY_CHUNK_SIZE) {
			ESP_LOGI(TAG, "Next chunk");
			spv_timestamp_t timestamp;
			timestamp = reading_start_time + reading * CONFIG_SPV_SENSOR_READ_FREQUENCY_SECONDS;

			size_t i = 0;
			cJSON *json = cJSON_CreateArray();
			for(; i + reading < reading_count && i < CONFIG_SPV_TELEMETRY_CHUNK_SIZE; i++) {
				ESP_LOGI(TAG, "Reading data %zu", i + reading);
				readings.noise = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_NOISE_READING);
				readings.luminosity = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_LUMINOSITY_READING);
				readings.co2 = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_CO2_READING);
				readings.voc = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_VOC_READING);
				readings.humidity = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_HUMIDITY_READING);
				readings.temperature = spv_ulp_get_reading_cvt(reading + i, SPV_ULP_TEMPERATURE_READING);

				cJSON *reading_json = spv_telemetry_reading_to_json(
					&readings,
					config->name,
					timestamp
				);
				cJSON_AddItemToArray(json, reading_json);
				timestamp += CONFIG_SPV_SENSOR_READ_FREQUENCY_SECONDS;
			}

			thingsboard_gateway_send_telemetry(
				&thingsboard_handle,
				json
			);
			cJSON_free(json);
		}
	}


	gateway_status.connectivity = connectivity;
	gateway_status.thingsboard_handle = &thingsboard_handle;
	if(connectivity) {
		ESP_ERROR_CHECK(wmesh_register_service(handle, &telemetry_service_config));
	}
	ESP_ERROR_CHECK(wmesh_register_service(handle, &ota_service_config));
	for(size_t i = 0; i < 3; i++) {
		ESP_LOGI(TAG, "Sending advertisement");
		spv_gateway_advertisement_t advertisement = {
			.current_time = time_get(),
			.firmware_version = spv_ota_get_current_version(),
			.flags = {
				.has_connectivity = connectivity,
			}
		};
		spv_gateway_send_advertisement(handle, &advertisement);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	gateway_status.last_telemetry = time_get();
	while(true) {
		vTaskDelay(pdMS_TO_TICKS(100));
		if(time_get() - gateway_status.last_telemetry >= 20) {
			break;
		}
	}

	if(gateway_status.ota_requested) {
		gateway_perform_ota(handle, &gateway_status);
	}

	ESP_LOGI(TAG, "Sending sleep advertisement");
	spv_timestamp_t wake_time = time_get() + CONFIG_SPV_WAKE_INTERVAL_MINUTES * 60;
	spv_gateway_sleep_command_t sleep = {
		.wake_time = wake_time,
	};
	spv_gateway_send_sleep(handle, &sleep);

	wmesh_stop(handle);
	nvs_flash_deinit();
	spv_ulp_start();

	if(connectivity) {
		ESP_LOGI(TAG, "Checking for updates...");
		thingsboard_ota_info_t ota_info;

		if((err = thingsboard_ota_info_get(&thingsboard_handle, &ota_info)) == ESP_OK) {
			if(strtoll(ota_info.version, NULL, 10) > spv_ota_get_current_version()) {
				ESP_LOGI(TAG, "Found newer firmware, updating");
				spv_ota_download(ota_info.http_url);
			} else {
				ESP_LOGI(TAG, "No updates found");
			}

			thingsboard_ota_info_free(&ota_info);
		} else {
			ESP_LOGE(TAG, "Error getting update");
		}
	}

	thingsboard_disconnect(&thingsboard_handle);
	esp_wifi_stop();

	spv_timestamp_t current_time = time_get();
	spv_timestamp_t sleep_seconds = current_time > wake_time ? 0 : wake_time - current_time;
	sleep_seconds *= 1.1;
	ESP_LOGI(TAG, "Sleeping for %"PRIu64 " seconds", sleep_seconds);
	esp_deep_sleep(sleep_seconds * 1000 * 1000 + 10);
}


static esp_err_t telemetry_cb(wmesh_handle_t *handle, wmesh_address_t src, uint8_t *data, size_t data_size, void *user_ctx){
	spv_telemetry_received_message_t msg = spv_telemetry_decode_message(data,data_size);
	gateway_status_t *gateway_status = user_ctx;

	switch (msg.type)
	{
	case TELEMETRY_TYPE_MSG:
		gateway_status->last_telemetry = time_get();
		ESP_LOGI(TAG,
				"Received telemetry msg from %02X:%02X:%02X:%02X:%02X:%02X",
				src[0], src[1], src[2], src[3], src[4], src[5]
			);

		ESP_LOGI(TAG, "	Name:		%s", msg.msg->node_name);
		ESP_LOGI(TAG, "	Date:		%"PRIu64, msg.msg->fecha);
		ESP_LOGI(TAG, "	Num_datos:	%"PRIu16, msg.msg->num_datos);

		cJSON *json = spv_telemetry_msg_to_json(msg.msg);
		thingsboard_gateway_send_telemetry(
			gateway_status->thingsboard_handle,
			json
		);
		cJSON_free(json);

		return ESP_OK;

	default:
		ESP_LOGE(TAG, "Received telemetry error");
		return ESP_FAIL;
	}
}


static esp_err_t ota_cb(wmesh_handle_t *handle, wmesh_address_t src, uint8_t *data, size_t data_size, void *user_ctx) {
	spv_ota_received_message_t msg = spv_ota_decode_message(data, data_size);
	gateway_status_t *gateway_status = user_ctx;

	switch (msg.type) {
		case OTA_TYPE_REQUEST:
			ESP_LOGI(TAG,
				"OTA update requested by %02X:%02X:%02X:%02X:%02X:%02X",
				src[0], src[1], src[2], src[3], src[4], src[5]
			);
			gateway_status->ota_requested = true;

			return ESP_OK;


		case OTA_TYPE_RETRY:
			if(gateway_status->next_chunk < msg.retry->restart_from) {
				ESP_LOGW(TAG, "Retry requested for future chunk");
				return ESP_OK;
			}

			ESP_LOGI(TAG,
				"Resetting chunk count to [%"PRIu64"] from [%zu]",
				msg.retry->restart_from, gateway_status->next_chunk
			);
			gateway_status->next_chunk = msg.retry->restart_from;
			gateway_status->retry_requested = true;
			gateway_status->chunks_since_retry = 0;
			return ESP_OK;

		default:
			return ESP_OK;
	}
}


static void gateway_perform_ota(wmesh_handle_t *handle, gateway_status_t *gateway_status) {
	esp_image_metadata_t metadata;
	const esp_partition_t *partition = esp_ota_get_running_partition();


	esp_image_get_metadata(
		&(esp_partition_pos_t) {
			.offset = partition->address,
			.size = partition->size
		},
		&metadata
	);
	spv_ota_send_begin(
		handle,
		&(spv_ota_begin_t) {
			.size = metadata.image_len,
			.version = spv_ota_get_current_version()
		}
	);

	size_t chunk_size = CONFIG_SPV_OTA_SERVICE_CHUNK_SIZE;
	size_t chunk_count = (metadata.image_len + chunk_size - 1) / chunk_size;
	gateway_status->next_chunk = 0;

	ESP_LOGI(TAG, "Starting OTA:");
	ESP_LOGI(TAG, "	Version:	%"PRIu64, spv_ota_get_current_version());
	ESP_LOGI(TAG, "	Size:		%zu Bytes", metadata.image_len);
	ESP_LOGI(TAG, "	Chunks:		%zu", chunk_count);

	vTaskDelay(pdMS_TO_TICKS(2000));
	spv_ota_data_t data_msg = { 0 };
	uint32_t wait_ms = 15;
	size_t last_chunk_sent = 0;
	for(
		gateway_status->next_chunk = 0;
		gateway_status->next_chunk < chunk_count && last_chunk_sent < 10;
		gateway_status->next_chunk += gateway_status->next_chunk == chunk_count - 1 ?
			0 : 1
	) {
		vTaskDelay(pdMS_TO_TICKS(wait_ms));
		if(gateway_status->retry_requested && wait_ms < 200) {
			uint32_t old_wait = wait_ms;
			wait_ms *= 1.1;
			ESP_LOGW(TAG,
				"Retry requested, slowing down: %"PRIu32"ms -> %"PRIu32 "ms",
				old_wait, wait_ms
			);
			gateway_status->retry_requested = false;
			vTaskDelay(pdMS_TO_TICKS(200));
		} else if(gateway_status->chunks_since_retry >= 100 && wait_ms > 25) {
			uint32_t old_wait = wait_ms;
			wait_ms /= 1.1;
			ESP_LOGI(TAG,
				"Connection deemed stable, speeding up: %"PRIu32"ms -> %"PRIu32"ms",
				old_wait, wait_ms
			);
			gateway_status->chunks_since_retry = 0;
		}

		size_t send_chunk = gateway_status->next_chunk;
		data_msg.chunk = send_chunk;
		size_t send_byte_offset = send_chunk * chunk_size;
		size_t send_bytes = send_chunk == chunk_count - 1 ?
			metadata.image_len - send_byte_offset :
			chunk_size;

		esp_partition_read(partition, send_byte_offset, data_msg.data, send_bytes);

		if(send_chunk % 10 == 0 || send_chunk == chunk_count - 1) {
			ESP_LOGI(TAG, "Sending OTA update chunk [%zu/%zu]", send_chunk, chunk_count - 1);
		}

		if(send_chunk == chunk_count - 1) {
			last_chunk_sent++;
			vTaskDelay(pdMS_TO_TICKS(200));
		} else {
			last_chunk_sent = 0;
		}

		gateway_status->chunks_since_retry++;

		spv_ota_send_data(handle, &data_msg);
	}

	vTaskDelay(pdMS_TO_TICKS(2000));
	spv_ota_send_end(handle);
	vTaskDelay(pdMS_TO_TICKS(10000));
}

#include "thingsboard/thingsboard.h"

#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "Thingsboard Connect";


thingsboard_connect_err_t thingsboard_connect(
	const thingsboard_config_t *config,
	thingsboard_handle_t *handle
) {
	const char *hostname = config->server ? config->server : CONFIG_THINGSBOARD_BROKER_HOSTNAME_DEFAULT;
	const esp_mqtt_client_config_t mqtt_cfg = {
		.credentials = {
			.username = config->api_key,
		},
		.broker.address = {
			.hostname = hostname,
			.transport = MQTT_TRANSPORT_OVER_TCP,
			.port = 1883
		}
	};

	esp_mqtt_client_handle_t mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);
	esp_err_t err;

	if((err = esp_mqtt_client_start(mqtt_handle)) != ESP_OK) {
		ESP_LOGE(TAG, "Error initializing MQTT connection: %s", esp_err_to_name(err));
		esp_mqtt_client_destroy(mqtt_handle);
		return THINGSBOARD_CONNECT_FAIL;
	}

	handle->mqtt_handle = mqtt_handle;

	size_t url_length = snprintf(
		NULL, 0, "https://%s/api/v1/%s/",
		hostname, config->api_key
	);

	char *url = malloc(url_length + 1);
	assert(url);
	sprintf(
		url, "https://%s/api/v1/%s/",
		hostname, config->api_key
	);
	handle->base_url = url;

	ESP_LOGI(TAG, "Connected to Thingsboard API");
	return THINGSBOARD_CONNECT_OK;
}


void thingsboard_disconnect(
	thingsboard_handle_t *handle
) {
	esp_mqtt_client_stop(handle->mqtt_handle);
	esp_mqtt_client_destroy(handle->mqtt_handle);

	ESP_LOGI(TAG, "Disconnected from Thingsboard API");
}

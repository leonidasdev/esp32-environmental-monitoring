#include "thingsboard/thingsboard.h"

#include "esp_log.h"

static const char *TAG = "Thingsboard GW Telemetry";


void thingsboard_gateway_send_telemetry(
	thingsboard_handle_t *handle,
	cJSON *json
) {
	char *data = cJSON_Print(json);
	if(!data) {
		ESP_LOGE(TAG, "Error printing JSON");
		return;
	}

	int err = esp_mqtt_client_publish(
		handle->mqtt_handle,
		"v1/devices/me/telemetry",
		data, strlen(data),
		0, 0
	);

	if(err == -1) {
		ESP_LOGE(TAG, "Error sending telemetry");
	} else if(err == -2) {
		ESP_LOGE(TAG, "Error sending telemetry: outbox full");
	}

	return;
}

#include "config/persistence.h"

#include "esp_log.h"
#include "nvs_flash.h"

#define NVS_PARTITION ("spv_config")
#define NVS_CONFIG_KEY ("config")

static const char *TAG = "SPV Config";


esp_err_t spv_config_read(spv_config_t *cfg) {
	esp_err_t err;
	nvs_handle_t handle;
	if((err = nvs_open(NVS_PARTITION, NVS_READWRITE, &handle)) != ESP_OK) {
		ESP_LOGE(TAG, "Error opening NVS partition: %s", esp_err_to_name(err));
		return err;
	}

	size_t out_length = sizeof(*cfg);
	err = nvs_get_blob(handle, NVS_CONFIG_KEY, cfg, &out_length);
	nvs_close(handle);

	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error reading configuration: %s", esp_err_to_name(err));
		return err;
	}

	ESP_LOGI(TAG, "Successfully read config:");
	ESP_LOGI(TAG, "	Device name: %s", cfg->name);
	ESP_LOGI(TAG, "	Role: %s", cfg->role == SPV_CONFIG_ROLE_GATEWAY ? "Gateway" : "Node");
	if(cfg->role == SPV_CONFIG_ROLE_GATEWAY) {
		ESP_LOGI(TAG, "	MQTT: %s", cfg->gateway_config.mqtt_key);
		ESP_LOGI(TAG, "	Wi-Fi SSID: %s", cfg->gateway_config.wifi_ssid);
		ESP_LOGI(TAG, "	Wi-Fi password: %s", cfg->gateway_config.wifi_password);
	}

	return err;
}


esp_err_t spv_config_write(const spv_config_t *cfg) {
	esp_err_t err;
	nvs_handle_t handle;
	if((err = nvs_open(NVS_PARTITION, NVS_READWRITE, &handle)) != ESP_OK) {
		ESP_LOGE(TAG, "Error opening NVS partition: %s", esp_err_to_name(err));
		return err;
	}

	err = nvs_set_blob(handle, NVS_CONFIG_KEY, cfg, sizeof(*cfg));
	nvs_commit(handle);
	nvs_close(handle);

	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error writing configuration: %s", esp_err_to_name(err));
	}

	return err;
}

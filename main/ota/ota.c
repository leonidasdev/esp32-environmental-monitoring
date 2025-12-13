#include "ota/ota.h"

#include "esp_log.h"

static const char *TAG = "SPV OTA";


esp_err_t spv_ota_begin(size_t ota_size, esp_ota_handle_t *handle) {

	ESP_LOGI(TAG, "Starting OTA update");

	const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
	if(!ota_partition) {
		ESP_LOGE(TAG, "Could not find available OTA partition");
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "Using OTA partition: %s", ota_partition->label);

	esp_err_t err = esp_ota_begin(ota_partition, ota_size, handle);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error starting OTA update: %s", esp_err_to_name(err));
		return ESP_FAIL;
	}

	return ESP_OK;
}


esp_err_t spv_ota_abort(esp_ota_handle_t handle) {
	ESP_LOGE(TAG, "OTA update aborted");
	esp_ota_abort(handle);

	return ESP_OK;
}


esp_err_t spv_ota_end(esp_ota_handle_t handle) {
	ESP_LOGI(TAG, "Finishing OTA update");

	esp_err_t err = esp_ota_end(handle);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error finishing OTA update: %s", esp_err_to_name(err));
		return err;
	}

	const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
	if(!ota_partition) {
		ESP_LOGE(TAG, "Error getting OTA partition");
		return ESP_FAIL;
	}
	err = esp_ota_set_boot_partition(ota_partition);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error setting boot partition: %s", esp_err_to_name(err));
		return err;
	}

	return ESP_OK;
}


uint64_t spv_ota_get_current_version() {
	return strtoll(esp_app_get_description()->version, NULL, 10);
}

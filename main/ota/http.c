#include "ota/ota.h"

#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "esp_log.h"


static const char *TAG = "SPV OTA HTTPS";


esp_err_t spv_ota_download(const char *url) {

	esp_http_client_config_t http_client_config = {
		.url = url,
		.crt_bundle_attach = esp_crt_bundle_attach
	};

	esp_https_ota_config_t config = {
		.http_config = &http_client_config
	};

	ESP_LOGI(TAG, "Performing OTA update...");
	esp_err_t err = esp_https_ota(&config);

	if(err == ESP_OK) {
		ESP_LOGI(TAG, "OTA Update successful");
		return err;
	} else {
		ESP_LOGE(TAG, "OTA update error: %s", esp_err_to_name(err));
		return ESP_FAIL;
	}
}

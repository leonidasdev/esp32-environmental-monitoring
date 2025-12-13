#include "config/provisioning.h"

#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"

#include "config/persistence.h"
#include "webserver/fs/persistence.h"
#include "webserver/webserver.h"
#include "wifi/wifi.h"

static const char *TAG = "SPV Provisioning";


esp_err_t spv_provisioning_start() {

	ESP_LOGI(TAG, "Starting provisioning service");

	ESP_ERROR_CHECK(spv_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(spv_wifi_ap_configure(&(spv_wifi_ap_options_t) {
		.ssid = CONFIG_SPV_PROVISIONING_WIFI_SSID,
		#ifdef CONFIG_SPV_PROVISIONING_PASSWORD
			.password = CONFIG_SPV_PROVISIONING_PASSWORD,
		#endif
	}));
	ESP_ERROR_CHECK(spv_wifi_start());

	fat32_mount("/fs", "storage");
	struct webserver_handle *handle = webserver_start("/fs/web");
	xEventGroupWaitBits(handle->event_group,
		WEBSERVER_POST_EVENT,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY
	);

	vTaskDelay(pdMS_TO_TICKS(5000));
	webserver_stop(handle);

	return ESP_OK;
}

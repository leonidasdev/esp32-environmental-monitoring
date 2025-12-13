#include <stdio.h>
#include "esp_log.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "wmesh/wmesh.h"

#include "config/persistence.h"
#include "config/provisioning.h"
#include "nodes/gateway.h"
#include "nodes/node.h"
#include "services/gateway.h"
#include "services/telemetry.h"
#include "sensors/sensors.h"
#include "sensors/ulp.h"
#include "time/clock.h"
#include "wifi/wifi.h"

static const char *TAG = "Main";


void app_main(void) {
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(spv_wifi_init());

	spv_config_t node_config;
	if(spv_config_read(&node_config) != ESP_OK) {
		ESP_LOGI(TAG, "Node not provisioned, starting provisioning...");
		spv_provisioning_start();
		esp_restart();
	}

	if(node_config.role == SPV_CONFIG_ROLE_NODE) {
		spv_start_node(&node_config);
	} else if(node_config.role == SPV_CONFIG_ROLE_GATEWAY) {
		spv_start_gateway(&node_config);
	} else {
		abort();
	}
}

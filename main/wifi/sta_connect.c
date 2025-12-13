#include "wifi/wifi.h"

#include "esp_log.h"
#include "freertos/event_groups.h"

static const char *TAG = "SPV Wi-Fi STA";

static EventGroupHandle_t wifi_event_group;
static int32_t wifi_retry_num = 0;

#define STATION_CONNECTED_BIT BIT0
#define STATION_FAIL_BIT      BIT1


static void sta_connect_handler(
	void *arg,
	esp_event_base_t event_base,
	int32_t event_id,
	void *event_data
);


esp_err_t spv_wifi_start_sta_connect(const spv_wifi_sta_options_t *options) {
	wifi_config_t sta_config = { 0 };
	esp_err_t err;

	if(strlen(options->ssid) >= sizeof(sta_config.sta.ssid)) {
		ESP_LOGE(TAG,
			"SSID too long: %zu. Max length is: %zu",
			strlen(options->ssid),
			sizeof(sta_config.sta.ssid) - 1
		);
		return ESP_ERR_INVALID_ARG;
	}

	if(options->password && strlen(options->password) >= sizeof(sta_config.sta.password)) {
		ESP_LOGE(TAG,
			"Password too long: %zu. Max length is: %zu",
			strlen(options->password),
			sizeof(sta_config.sta.password) - 1
		);
		return ESP_ERR_INVALID_ARG;
	}

	strcpy((char *) sta_config.sta.ssid, options->ssid);
	if(options->password) {
		strcpy((char *) sta_config.sta.password, options->password);
		sta_config.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;
	} else {
		sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
	}

	if(options->bssid) {
		memcpy(sta_config.sta.bssid, options->bssid, sizeof(sta_config.sta.bssid));
	}

	wifi_retry_num = options->retries == 0 ? -1 : options->retries;
	wifi_event_group = xEventGroupCreate();
	if(wifi_event_group == NULL) {
		ESP_LOGE(TAG, "Error allocating event group.");
		return ESP_ERR_NO_MEM;
	}

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;

	err = esp_event_handler_instance_register(
		WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&sta_connect_handler,
		NULL,
		&instance_any_id
	);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error registering event handler: %s", esp_err_to_name(err));
		return err;
	}

	err = esp_event_handler_instance_register(
		IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&sta_connect_handler,
		NULL,
		&instance_got_ip
	);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error registering event handler: %s", esp_err_to_name(err));
		esp_event_handler_instance_unregister(
			WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id
		);
		return err;
	}

	if((err = esp_wifi_set_config(WIFI_IF_STA, &sta_config)) != ESP_OK) {
		ESP_LOGE(TAG, "Error setting STA configuration: %s", esp_err_to_name(err));
		esp_event_handler_instance_unregister(
			IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip
		);
		esp_event_handler_instance_unregister(
			WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id
		);
		return err;
	}

	if((err = spv_wifi_start()) != ESP_OK) {
		ESP_LOGE(TAG, "Error starting Wi-Fi modem: %s", err);
		esp_event_handler_instance_unregister(
			IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip
		);
		esp_event_handler_instance_unregister(
			WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id
		);
		return err;
	}

	esp_wifi_connect();

	ESP_LOGI(TAG, "Connecting to Wi-Fi network: %s", options->ssid);
	EventBits_t bits = xEventGroupWaitBits(
		wifi_event_group,
		STATION_CONNECTED_BIT | STATION_FAIL_BIT,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY
	);

	esp_event_handler_instance_unregister(
		IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip
	);
	esp_event_handler_instance_unregister(
		WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id
	);

	vEventGroupDelete(wifi_event_group);

	if(bits & STATION_CONNECTED_BIT) {
		ESP_LOGI(TAG, "Wi-Fi connection successful");
		return ESP_OK;
	}

	if(bits & STATION_FAIL_BIT) {
		ESP_LOGE(TAG, "Error connecting to Wi-Fi network");
		return ESP_ERR_WIFI_NOT_ASSOC;
	}

	return ESP_FAIL;
}


static void sta_connect_handler(
	void *arg,
	esp_event_base_t event_base,
	int32_t event_id,
	void *event_data
) {

	if(event_base == WIFI_EVENT) {
		switch(event_id) {
			case WIFI_EVENT_STA_START:
				esp_wifi_connect();
				return;

			case WIFI_EVENT_STA_DISCONNECTED:
				if(wifi_retry_num == 0) {
					xEventGroupSetBits(wifi_event_group, STATION_FAIL_BIT);
					return;
				}

				ESP_LOGW(TAG, "Retrying to connect, %"PRIu32" attempts left", wifi_retry_num);
				esp_wifi_connect();
				wifi_retry_num--;
				return;

			default:
				return;
		}
	}

	if(event_base == IP_EVENT) {
		switch(event_id) {
			case IP_EVENT_STA_GOT_IP:
				xEventGroupSetBits(wifi_event_group, STATION_CONNECTED_BIT);
				return;

			default:
				return;
		}
	}
}

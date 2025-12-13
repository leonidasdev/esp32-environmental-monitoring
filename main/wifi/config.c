#include "wifi/wifi.h"

#include "esp_log.h"

static const char *TAG = "SPV Wifi Config";
static const char *wifi_mode_names[] = {
	[WIFI_MODE_NULL] = "WIFI_MODE_NULL",
	[WIFI_MODE_STA] = "WIFI_MODE_STA",
	[WIFI_MODE_AP] = "WIFI_MODE_AP",
	[WIFI_MODE_APSTA] = "WIFI_MODE_APSTA",
	[WIFI_MODE_NAN] = "WIFI_MODE_NAN",
	[WIFI_MODE_MAX] = "WIFI_MODE_MAX",
};

esp_err_t spv_wifi_init() {
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	esp_err_t err;
	if((err = esp_netif_init()) != ESP_OK) {
		ESP_LOGE(TAG, "Error initializing TCP/IP stack: %s", esp_err_to_name(err));
		return err;
	}


	if((err = esp_wifi_init(&wifi_init_config)) != ESP_OK) {
		ESP_LOGE(TAG, "Error initializing Wi-Fi driver: %s", esp_err_to_name(err));
		return err;
	}

	ESP_LOGI(TAG, "Wi-Fi driver initialized");
	return ESP_OK;
}


esp_err_t spv_wifi_set_mode(wifi_mode_t wifi_mode) {
	switch(wifi_mode) {
		case WIFI_MODE_AP:
			esp_wifi_set_mode(wifi_mode);
			ESP_LOGI(TAG, "Wi-Fi configured as AP");
			return ESP_OK;

		case WIFI_MODE_STA:
			esp_wifi_set_mode(wifi_mode);
			esp_netif_create_default_wifi_sta();
			ESP_LOGI(TAG, "Wi-Fi configured as STA");
			return ESP_OK;

		case WIFI_MODE_APSTA:
			esp_wifi_set_mode(wifi_mode);
			esp_netif_create_default_wifi_sta();
			ESP_LOGI(TAG, "Wi-Fi configured as AP+STA");
			return ESP_OK;

		default:
			ESP_LOGE(TAG,
				"Invalid Wi-Fi mode: %s",
				wifi_mode_names[wifi_mode]
			);

			ESP_LOGE(TAG, "Compatible modes are:");
			ESP_LOGE(TAG, "	- WIFI_MODE_AP");
			ESP_LOGE(TAG, "	- WIFI_MODE_STA");
			ESP_LOGE(TAG, "	- WIFI_MODE_APSTA");
			return ESP_ERR_INVALID_ARG;
	}
}


esp_err_t spv_wifi_ap_configure(const spv_wifi_ap_options_t *options) {

	esp_netif_create_default_wifi_ap();
	esp_err_t err;
	wifi_mode_t current_mode;
	if((err = esp_wifi_get_mode(&current_mode)) != ESP_OK) {
		ESP_LOGE(TAG, "Error reading current Wi-Fi mode: %s", esp_err_to_name(err));
		return err;
	}

	if(current_mode != WIFI_MODE_AP && current_mode != WIFI_MODE_APSTA) {
		ESP_LOGE(TAG,
			"Invalid Wi-Fi mode for AP: %s",
			wifi_mode_names[current_mode]
		);

		ESP_LOGE(TAG, "Compatible modes are:");
		ESP_LOGE(TAG, "	- WIFI_MODE_AP");
		ESP_LOGE(TAG, "	- WIFI_MODE_APSTA");
		return ESP_ERR_INVALID_STATE;
	}


	wifi_config_t ap_config = {
		.ap = {
			.authmode = options->password == NULL ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA3_PSK,
			.ssid_hidden = options->hidden,
			.max_connection = 8
		}
	};

	if(strlen(options->ssid) >= sizeof(ap_config.ap.ssid)) {
		ESP_LOGE(TAG,
			"AP SSID too long: %zu. Maximum allowed length is: %zu",
			strlen(options->ssid),
			sizeof(ap_config.ap.ssid) - 1
		);
		return ESP_ERR_INVALID_ARG;
	}

	if(options->password && strlen(options->password) >= sizeof(ap_config.ap.password)) {
		ESP_LOGE(TAG,
			"AP password too long: %zu. Maximum allowed length is: %zu",
			strlen(options->password),
			sizeof(ap_config.ap.password) - 1
		);
		return ESP_ERR_INVALID_ARG;
	}

	strcpy((char *) ap_config.ap.ssid, options->ssid);
	if(options->password) {
		strcpy((char *) ap_config.ap.password, options->password);
	}

	if((err = esp_wifi_set_config(WIFI_IF_AP, &ap_config)) != ESP_OK) {
		ESP_LOGE(TAG, "Error setting AP config: %s", esp_err_to_name(err));
		return err;
	}

	return ESP_OK;
}


esp_err_t spv_wifi_start() {
	esp_err_t err;
	if((err = esp_wifi_start()) != ESP_OK) {
		ESP_LOGE(TAG, "Error starting Wi-Fi: %s", esp_err_to_name(err));
		return err;
	}

	return ESP_OK;
}


void spv_wifi_set_channel(uint8_t channel) {
	esp_wifi_set_channel(channel, 0);
}

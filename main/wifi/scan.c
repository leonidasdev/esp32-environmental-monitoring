#include "wifi.h"

#include "esp_log.h"

static const char *TAG = "SPV Wi-Fi Scan";


esp_err_t spv_wifi_scan(
	spv_wifi_scan_results_t *result,
	const spv_wifi_scan_options_t *scan_options
) {
	result->network_count = 0;
	result->networks = NULL;

	wifi_scan_config_t scan_config = {
		.ssid = NULL,
		.bssid = NULL,
		.channel = 0,
		.show_hidden = false,
		.scan_type = WIFI_SCAN_TYPE_ACTIVE,
		.scan_time = {
			.active.max = 40,
			.active.min = 40
		},
	};

	ESP_LOGI(TAG, "Performing Wi-Fi scan");
	esp_err_t err = esp_wifi_scan_start(&scan_config, true);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error performing Wi-Fi scan: %s", esp_err_to_name(err));
		return err;
	}

	uint16_t ap_count;
	if((err = esp_wifi_scan_get_ap_num(&ap_count)) != ESP_OK) {
		ESP_LOGE(TAG, "Error getting scan results: %s", esp_err_to_name(err));
		return err;
	}

	if(ap_count == 0) {
		ESP_LOGI(TAG, "Wi-Fi scan found no networks");
		return ESP_OK;
	}

	wifi_ap_record_t *raw_records = malloc(sizeof(*raw_records) * ap_count);

	if(!raw_records) {
		ESP_LOGE(TAG, "Error allocating memory for AP records");
		return ESP_ERR_NO_MEM;
	}

	if((err = esp_wifi_scan_get_ap_records(&ap_count, raw_records)) != ESP_OK) {
		ESP_LOGE(TAG, "Error reading AP records: %s", esp_err_to_name(err));
		free(raw_records);
		return err;
	}


	size_t filtered_ap_count = ap_count;
	if(scan_options->ssid_filter) {
		for(size_t i = 0; i < ap_count; i++) {
			wifi_ap_record_t *record = &raw_records[i];
			const char **ssid;
			for(ssid = scan_options->ssid_filter; *ssid; ssid++) {
				if(strcmp(*ssid, (char*) record->ssid) == 0) {
					break;
				}
			}

			if(!*ssid) {
				filtered_ap_count--;
			}
		}
	}

	result->network_count = filtered_ap_count;
	if(filtered_ap_count == 0) {
		ESP_LOGI(TAG, "Wi-Fi scan found no networks");
		free(raw_records);
		return ESP_OK;
	}

	result->networks = calloc(sizeof(*result->networks) * filtered_ap_count, 1);
	if(!result->networks) {
		ESP_LOGE(TAG, "Error allocating memory for scan results");
		free(raw_records);
		return ESP_ERR_NO_MEM;
	}

	size_t j = 0;
	for(size_t i = 0; i < ap_count; i++) {
		wifi_ap_record_t *record = &raw_records[i];

		if(scan_options->ssid_filter) {
			const char **ssid;
			for(ssid = scan_options->ssid_filter; *ssid; ssid++) {
				if(strcmp(*ssid, (char*) record->ssid) == 0) {
					memcpy(result->networks[j].ssid, record->ssid, sizeof(record->ssid));
					memcpy(result->networks[j].bssid, record->bssid, sizeof(record->bssid));
					result->networks[j].auth_mode = record->authmode;
					result->networks[j].channel = record->primary;
					j++;
				}
			}
		} else {
			memcpy(result->networks[j].ssid, record->ssid, sizeof(record->ssid));
			memcpy(result->networks[j].bssid, record->bssid, sizeof(record->bssid));
			result->networks[j].auth_mode = record->authmode;
			result->networks[j].channel = record->primary;
			j++;
		}
	}

	free(raw_records);

	ESP_LOGI(TAG, "Wi-Fi scan found %zu networks", filtered_ap_count);
	for(size_t i = 0; i < result->network_count; i++) {
		ESP_LOGI(TAG, "%s:", result->networks[i].ssid);
		ESP_LOGI(TAG, "	Channel:%"PRIu8, result->networks[i].channel);
		const char *auth_mode_name[] = {
			[WIFI_AUTH_OPEN] = "Open",
			[WIFI_AUTH_WEP] = "WEP",
			[WIFI_AUTH_WPA_PSK] = "WPA",
			[WIFI_AUTH_WPA2_PSK] = "WPA2",
			[WIFI_AUTH_WPA_WPA2_PSK] = "WPA+WPA2",
			[WIFI_AUTH_WPA2_ENTERPRISE] = "WPA2 Enterprise",
			[WIFI_AUTH_WPA3_PSK] = "WPA3",
			[WIFI_AUTH_WPA2_WPA3_PSK] = "WPA2+WPA3",
			[WIFI_AUTH_WAPI_PSK] = "WAPI",
			[WIFI_AUTH_OWE] = "OWE",
			[WIFI_AUTH_WPA3_ENT_192] = "WPA3 Enterprise 192 Bits",
			[WIFI_AUTH_DPP] = "DPP",
			[WIFI_AUTH_WPA3_ENTERPRISE] = "WPA3 Enterprise",
			[WIFI_AUTH_WPA2_WPA3_ENTERPRISE] = "WPA2+WPA3 Enterprise",
			[WIFI_AUTH_WPA_ENTERPRISE] = "WPA Enterprise",
		};
		ESP_LOGI(TAG, "	Auth:	%s", auth_mode_name[result->networks[i].auth_mode]);

		char *bssid = result->networks[i].bssid;
		ESP_LOGI(TAG, "	BSSID:	%2X:%2X:%2X:%2X:%2X:%2X",
			bssid[0], bssid[1], bssid[2],
			bssid[3], bssid[4], bssid[5]
		);
	}
	return ESP_OK;
}


void spv_wifi_scan_free(spv_wifi_scan_results_t *result) {
	free(result->networks);
	result->network_count = 0;
	result->networks = NULL;
}

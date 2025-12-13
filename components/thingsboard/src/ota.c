#include "thingsboard/thingsboard.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "http/http.h"

static const char *TAG = "Thingsboard OTA";


esp_err_t thingsboard_ota_info_get(
	thingsboard_handle_t *handle,
	thingsboard_ota_info_t *info
) {
	ESP_LOGI(TAG, "Retrieving OTA data...");

	const char *uri[] = {
		handle->base_url, "attributes?sharedKeys=fw_title,fw_version,fw_size",
		0
	};

	struct api_http_response http_response;
	esp_err_t result = api_http_get(
		uri,
		&http_response,
		true,
		10000
	);

	if(result != ESP_OK) {
		ESP_LOGE(TAG, "Error connecting to Thingsboard");
		return ESP_FAIL;
	}

	if(http_response.code != 200) {
		ESP_LOGE(TAG, "Thingsboard returned %d error", http_response.code);
		api_http_response_free(&http_response);
		return ESP_FAIL;
	}

	cJSON *json = cJSON_Parse(http_response.response);
	api_http_response_free(&http_response);

	if(json == NULL) {
		ESP_LOGE(TAG, "Malformed response");
		return ESP_FAIL;
	}

	/* Response structure is:
	 * {
	 *		"shared": {
	 *			"fw_size": <Size in bytes>,
	 *			"fw_title": <Update name>,
	 *			"fw_version": <Update version>
	 *		}
	 * }
	 */
	cJSON *json_shared = cJSON_GetObjectItem(json, "shared");
	if(json_shared == NULL) {
		ESP_LOGI(TAG, "No firmware data");
		cJSON_free(json);
		return ESP_FAIL;
	}

	cJSON *json_fw_size = cJSON_GetObjectItem(json_shared, "fw_size");
	cJSON *json_fw_title = cJSON_GetObjectItem(json_shared, "fw_title");
	cJSON *json_fw_version = cJSON_GetObjectItem(json_shared, "fw_version");

	if(!json_fw_size || !json_fw_title || !json_fw_version) {
		ESP_LOGE(TAG, "Malformed response");
		cJSON_free(json);
		return ESP_FAIL;
	}

	size_t fw_size = cJSON_GetNumberValue(json_fw_size);
	char *fw_title = cJSON_GetStringValue(json_fw_title);
	char *fw_version = cJSON_GetStringValue(json_fw_version);


	info->name = fw_title;
	info->size = fw_size;
	info->version = fw_version;

	size_t fw_url_length = snprintf(
		NULL, 0, "%sfirmware?title=%s&version=%s",
		handle->base_url, fw_title, fw_version
	);
	char *fw_url = malloc(fw_url_length + 1);
	assert(fw_url);

	sprintf(
		fw_url, "%sfirmware?title=%s&version=%s",
		handle->base_url, fw_title, fw_version
	);

	info->http_url = fw_url;

	ESP_LOGI(TAG, "Found firmware:");
	ESP_LOGI(TAG, "\tName:\t\t%s", fw_title);
	ESP_LOGI(TAG, "\tVersion:\t%s", fw_version);
	return ESP_OK;
}


void thingsboard_ota_info_free(
	thingsboard_ota_info_t *info
) {
	free(info->http_url);
	free(info->name);
	free(info->version);
}

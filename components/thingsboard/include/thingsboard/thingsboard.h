#ifndef THINGSBOARD_THINGSBOARD_H_
#define THINGSBOARD_THINGSBOARD_H_

#include "cJSON.h"
#include "esp_err.h"
#include "mqtt_client.h"
#include "sdkconfig.h"


/// @brief Thingsboard API handle.
typedef struct {

	/// @brief MQTT client handle.
	esp_mqtt_client_handle_t mqtt_handle;

	/// @brief Contains the Thingsboard URL with the API key:
	///
	/// `https://{domain}/api/v1/{api_key}/`
	char *base_url;

} thingsboard_handle_t;


/// @brief Thingsboard API configuration.
typedef struct {

	/// @brief Thingsboard API key.
	char *api_key;

	/// @brief Optional, defaults to `demo.thingsboard.io`. Set to change the
	/// API endpoint.
	char *server;

	/// @brief Set to enable Gateway API functions.
	bool is_gateway;

} thingsboard_config_t;


/// @brief Error type for `thingsboard_connect`.
typedef enum {
	/// @brief Connection successful.
	THINGSBOARD_CONNECT_OK = 0,

	/// @brief Error connecting to the server.
	THINGSBOARD_CONNECT_CONNECTION_ERROR,

	/// @brief Error authenticating with the API key.
	THINGSBOARD_CONNECT_API_KEY_ERROR,

	/// @brief Unknown error.
	THINGSBOARD_CONNECT_FAIL,
} thingsboard_connect_err_t;


/// @brief Configures and connects to the Thingsboard API.
///
/// @param[in] config Configuration parameters.
/// @param[out] handle Thingsboard handle. Only valid when
///		`THINGSBOARD_CONNECT_OK` is returned.
///		Pass to `thingsboard_disconnect` to free.
///
/// @return Error code. `handle` is only valid when `THINGSBOARD_CONNECT_OK` is
///		returned.
///
///	- `THINGSBOARD_CONNECT_OK`: Connection successful.
///
///	- `THINGSBOARD_CONNECT_CONNECTION_ERROR`: Error when connecting to the server.
///		Either a Wi-Fi error or IP error.
///
///	- `THINGSBOARD_CONNECT_API_KEY_ERROR`: Server rejected the API key.
///
///	- `THINGSBOARD_CONNECT_FAIL`: Internal error.
thingsboard_connect_err_t thingsboard_connect(
	const thingsboard_config_t *config,
	thingsboard_handle_t *handle
);


/// @brief Disconnect from the Thingsboard API and free all allocated resources.
///
/// @param[in] handle Connection handle.
void thingsboard_disconnect(
	thingsboard_handle_t *handle
);


void thingsboard_gateway_send_telemetry(
	thingsboard_handle_t *handle,
	cJSON *json
);


/// @brief OTA update data.
typedef struct {

	/// @brief Update title.
	/// @note Corresponds to `fw_title`.
	char *name;

	/// @brief Firmware version string.
	/// @note Corresponds to `fw_size`.
	char *version;

	/// @brief Update size in bytes.
	/// @note Corresponds to `fw_size`.
	size_t size;

	/// @brief Direct link to the OTA update. May be passed to
	/// 	`esp_https_ota`.
	char *http_url;

} thingsboard_ota_info_t;


/// @brief Fetches firmware information from Thingsboard.
///
/// @param[in] handle Thingsboard handle.
/// @param[out] info Latest firmware information. Only valid when `ESP_OK` is
///		returned. Pass to `thingsboard_ota_info_free` to free.
///
/// @return `ESP_OK` or error.
esp_err_t thingsboard_ota_info_get(
	thingsboard_handle_t *handle,
	thingsboard_ota_info_t *info
);


/// @brief Frees a `thingsboard_ota_info_t` struct.
void thingsboard_ota_info_free(
	thingsboard_ota_info_t *info
);

#endif

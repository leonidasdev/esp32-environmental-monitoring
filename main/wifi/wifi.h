#ifndef SPV_WIFI_WIFI_H_
#define SPV_WIFI_WIFI_H_

#include "esp_err.h"
#include "esp_wifi.h"


/// @brief Initializes the Wi-Fi modem.
///
/// @warning Must be called before `spv_wifi_start`.
///
/// @return `ESP_OK` or error.
esp_err_t spv_wifi_init();


/// @brief Sets the Wi-Fi modem to the selected mode.
///
/// @warning Must be called before `spv_wifi_start`.
///
/// @param[in] wifi_mode Modem mode. Accepted values:
///
///		- `WIFI_MODE_AP`
///
///		- `WIFI_MODE_STA`
///
///		- `WIFI_MODE_APSTA`
///
/// @return One of the following:
///
///		- `ESP_OK`: Mode set successfully.
///
///		- `ESP_ERR_INVALID_ARG`: If `wifi_mode` is an invalid variant.
///
///		- Other: In case of an error.
esp_err_t spv_wifi_set_mode(wifi_mode_t wifi_mode);


/// @brief Configuration options for `spv_wifi_ap_configure`.
typedef struct {

	/// @brief AP SSID.
	const char *ssid;

	/// @brief Optional. Network password.
	const char *password;

	/// @brief Set to true to hide the network's SSID.
	bool hidden;
} spv_wifi_ap_options_t;
/// @brief Configures the AP interface.
///
/// @param[in] options AP configuration.
///
/// @return `ESP_OK` or error.
esp_err_t spv_wifi_ap_configure(const spv_wifi_ap_options_t *options);


/// @brief Starts the Wi-Fi subsystem.
///
/// @return `ESP_OK` or error.
esp_err_t spv_wifi_start();


/// @brief Configuration options for `spv_wifi_sta_connect`.
typedef struct {

	/// @brief Network SSID.
	const char *ssid;

	/// @brief Optional. Network password. Set to `NULL` when connecting to an
	/// open network.
	const char *password;

	/// @brief Optional. Use to filter the AP by its MAC address.
	const char *bssid;

	/// @brief Amount of times to try to connect to the network before giving
	///		up. Set to `0` for infinite retries.
	uint8_t retries;

} spv_wifi_sta_options_t;
/// @brief Connects to a Wi-Fi network.
///
/// @note Modem must be set to either `WIFI_MODE_STA` or `WIFI_MODE_APSTA`.
///
/// @warning Automatically calls `spv_wifi_start`.
///
/// @param[in] options STA configuration.
///
/// @return One of the following:
///
///		- `ESP_OK`: When successful.
///
///		- `ESP_ERR_INVALID_STATE`: When Wi-Fi modem is not in STA mode.
///
///		- `ESP_ERR_WIFI_NOT_ASSOC`: When failing to connect after specified
///			retries.
///
///		- Other: In case of an internal error.
esp_err_t spv_wifi_start_sta_connect(const spv_wifi_sta_options_t *options);


/// @brief Sets the Wi-Fi modem to the given channel.
///
/// @param[in] channel Channel to set.
void spv_wifi_set_channel(uint8_t channel);


/// @brief Configuration options for `spv_wifi_scan`.
typedef struct {

	/// @brief Optional. `NULL`-terminated array of SSID strings. Set to filter
	/// scan results by SSID.
	const char **ssid_filter;

} spv_wifi_scan_options_t;


/// @brief Wi-Fi scan result.
typedef struct {

	/// @brief Amount of networks found.
	size_t network_count;

	/// @brief Network data.
	struct {

		/// @brief Network SSID.
		char ssid[33];

		/// @brief Network BSSID.
		char bssid[6];

		/// @brief AP channel.
		uint8_t channel;

		/// @brief Network authentication mode.
		wifi_auth_mode_t auth_mode;

	} *networks;

} spv_wifi_scan_results_t;
/// @brief Performs a scan of all available Wi-Fi networks.
///
/// @note Modem must be set to either `WIFI_MODE_STA` or `WIFI_MODE_APSTA`.
///
/// @warning Must be called after `spv_wifi_start`.
///
/// @param[out] result Scan results. Only valid when `ESP_OK` is returned. Pass
/// 	to `spv_wifi_scan_free` to free.
/// @param[in] scan_options Optional. Scan filters.
///
/// @return One of the following:
///
///		- `ESP_OK`: When successful.
///
///		- `ESP_ERR_INVALID_STATE`: When Wi-Fi modem is not in STA mode.
///
///		- Other: In case of an internal error.
esp_err_t spv_wifi_scan(
	spv_wifi_scan_results_t *result,
	const spv_wifi_scan_options_t *scan_options
);


/// @brief Frees a `spv_wifi_scan_results_t` struct.
void spv_wifi_scan_free(spv_wifi_scan_results_t *result);

#endif

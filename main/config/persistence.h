#ifndef SPV_CONFIG_PERSISTENCE_H_
#define SPV_CONFIG_PERSISTENCE_H_

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "sdkconfig.h"


/// @brief Configures whether the device is a node or a gateway.
typedef enum {
	SPV_CONFIG_ROLE_NODE,
	SPV_CONFIG_ROLE_GATEWAY,
} spv_config_role_t;


/// @brief Stores configuration for this node.
typedef struct {

	/// @brief Mesh network key.
	uint8_t mesh_key[CONFIG_WMESH_NETKEY_LENGTH];

	/// @brief Device name.
	char name[CONFIG_SPV_TELEMETRY_NODE_NAME_LENGTH];

	/// @brief Configures this node's role.
	spv_config_role_t role;

	union {

		/// @brief Configuration used when node is a gateway.
		struct {

			/// @brief WiFi SSID.
			char wifi_ssid[33];

			/// @brief WiFi password.
			char wifi_password[64];

			/// @brief Thingsboard API key.
			char mqtt_key[64];

		} gateway_config;

		/// @brief Used when acting as a node.
		struct {
		} node_config;
	};

} spv_config_t;


/// @brief Reads configuration into `cfg`.
///
/// @note NVS must be initialized before using this function.
///
/// @param[out] cfg Configuration output.
///
/// @return One of the following:
///
///	- `ESP_OK` if successful.
///
///	- `ESP_ERR_NVS_NOT_FOUND` if not configured.
///
///	- Other value in case of an error.
esp_err_t spv_config_read(spv_config_t *cfg);


/// @brief Stores the provided configuration.
///
/// @note NVS must be initialized before using this function.
///
/// @param cfg Configuration to store.
///
/// @return `ESP_OK` or error.
esp_err_t spv_config_write(const spv_config_t *cfg);

#endif

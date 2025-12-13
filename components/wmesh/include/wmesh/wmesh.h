#ifndef CUSTOM_WMESH_WMESH_H_
#define CUSTOM_WMESH_WMESH_H_

#include "sdkconfig.h"
#include <stdint.h>
#include <esp_err.h>

#include "wmesh/common.h"
#include "wmesh/encryption.h"
#include "wmesh/peer.h"
#include "wmesh/storage.h"


/// @brief Service identifier. Must be unique to a single service.
typedef uint8_t wmesh_service_id_t;


/// @brief Service callback.
///
/// @param[in] handle Mesh handle.
/// @param[in] data Received data buffer. May be used as a scratch buffer.
/// @param[in] data_size Number of received bytes.
/// @param[in] user_ctx Custom user data.
///
/// @return Return values other than `ESP_OK` will be treated as an error and
/// logged to the console.
typedef esp_err_t (*wmesh_service_recv_cb_t)(
	struct wmesh_handle_t *handle,
	wmesh_address_t src,
	uint8_t *data,
	size_t data_size,
	void *user_ctx
);


/// @brief Mesh service configuration.
typedef struct {

	/// @brief Called when a message is sent to this service.
	wmesh_service_recv_cb_t receive_callback;

	/// @brief User data. Passed to `receive_callback`.
	void *ctx;

	/// @brief Used to uniquely identify a service on the mesh network.
	/// @attention Some service numbers are reserved and may not be used.
	wmesh_service_id_t id;

} wmesh_service_config_t;


/// @brief Initial configuration for Wi-Fi mesh.
typedef struct {

	/// @brief Null-terminated array of initial services.
	/// May be modified after the fact using `wmesh_register_service` and
	/// `wmesh_uregister_service`.
	wmesh_service_config_t *service_config;


	/// @brief Shared network key.
	uint8_t network_key[CONFIG_WMESH_NETKEY_LENGTH];

	/// @brief Wi-Fi channel to use for the Mesh network. Set to 0 for
	/// automatic selection.
	///
	/// This option does nothing if Wi-Fi Station mode is already set-up and
	/// connected to a channel.
	int8_t wifi_channel;

} wmesh_config_t;


/// @brief Mesh service handle.
typedef struct wmesh_service_handle_t {

	/// @brief Called when a message is received.
	wmesh_service_recv_cb_t receive_callback;

	/// @brief User context.
	void *ctx;

	/// @brief Pointer to next handle.
	struct wmesh_service_handle_t *next;

	/// @brief Service ID.
	wmesh_service_id_t id;

} wmesh_service_handle_t;


/// @brief Mesh handle.
typedef struct wmesh_handle_t {

	/// @brief Used for encrypting messages.
	wmesh_encryption_ctx_t *encryption_ctx;

	/// @brief Known peer list.
	wmesh_peer_list_t *peers;

	/// @brief Pointer to service handlers.
	wmesh_service_handle_t *service_handlers;

	/// @brief Stores peer counters.
	wmesh_storage_handle_t peer_storage;

	/// @brief Stores self data.
	wmesh_storage_handle_t self_storage;

	/// @brief Sequence number, identifies a packet. Automatically increases
	/// when a message is sent.
	wmesh_encryption_ctr_t sequence_number;

	/// @brief Shared network key.
	uint8_t network_key[CONFIG_WMESH_NETKEY_LENGTH];

} wmesh_handle_t;


/// @brief Workaround for ESP-NOW.
extern wmesh_handle_t *wmesh_global_handle;


/// @brief Initializes and connects to a mesh network.
///
/// Configures ESP-NOW in Soft-AP mode. Application code may
/// use Station mode freely.
///
/// @note Wi-Fi modem must be configured and set to either SoftAP mode
/// (`WIFI_MODE_AP`) or SoftAP + STA (`WIFI_MODE_APSTA`) before calling this
/// function.
///
/// @attention Currently, only one mesh instance may be running at a time. Calling
/// this function multiple times without calling `wmesh_stop` in-between, *WILL*
/// lead to unexpected behavior.
///
/// @param[in] config Mesh configuration options.
///
/// @return Pointer to a handle, or `NULL` in case of an error.
wmesh_handle_t *wmesh_init(const wmesh_config_t *config);


/// @brief Uninitializes, and stops all mesh activities.
/// @param[in] handle Mesh handle to stop.
///
/// @return `ESP_OK` if successful.
esp_err_t wmesh_stop(wmesh_handle_t *handle);


/// @brief Sends a message at the mesh level.
/// @attention Messages sent using this function are not ACK'd, and, as such,
/// are not guaranteed to be received.
///
/// @param handle Mesh handle.
/// @param dest Node to send this message to.
/// @param data Data to send to the node.
/// @param data_length Size in bytes of the payload.
///
/// @return `ESP_OK` if successful.
esp_err_t wmesh_ll_send(
	wmesh_handle_t *handle, const wmesh_address_t dest,
	const uint8_t *data, size_t data_length
);


/// @brief Send a message at the service level.
///
/// @param handle Mesh handle.
/// @param dest Destination node address.
/// @param service Destination service ID.
/// @param data Data to send.
/// @param data_length Size of data in bytes.
///
/// @return `ESP_OK` or error.
esp_err_t wmesh_send(
	wmesh_handle_t *handle, const wmesh_address_t dest,
	wmesh_service_id_t service,
	const uint8_t *data, size_t data_length
);


/// @brief Adds a service handler to the mesh.
///
/// @param handle Mesh handle.
/// @param config Service configuration.
///
/// @return `ESP_OK` or error.
esp_err_t wmesh_register_service(
	wmesh_handle_t *handle, const wmesh_service_config_t *config
);


/// @brief Removes a service handler from the mesh.
///
/// @param handle Mesh handle.
/// @param id Service to remove.
///
/// @return `ESP_OK` or error.
esp_err_t wmesh_unregister_service(
	wmesh_handle_t *handle, wmesh_service_id_t id
);
#endif

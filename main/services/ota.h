#ifndef SPV_SERVICES_OTA_H_
#define SPV_SERVICES_OTA_H_

#include "sdkconfig.h"

#include <stdint.h>
#include "time/clock.h"
#include "wmesh/wmesh.h"


/// @brief Sent by a node to request an OTA update.
typedef struct __attribute__((packed)) {
	uint8_t _dummy;
} spv_ota_request_t;


/// @brief Used to advertise the start of an OTA update.
typedef struct __attribute__((packed)) {

	/// @brief Firmware version.
	uint64_t version;

	/// @brief Update size in bytes.
	size_t size;

} spv_ota_begin_t;


/// @brief Update data chunk.
typedef struct __attribute__((packed)) {

	/// @brief Chunk sequence identifier.
	uint64_t chunk;

	/// @brief Chunk data.
	uint8_t data[CONFIG_SPV_OTA_SERVICE_CHUNK_SIZE];

} spv_ota_data_t;


/// @brief Sent by a node to signal a lost packet.
typedef struct __attribute__((packed)) {

	/// @brief Which chunk to restart the update from.
	uint64_t restart_from;

} spv_ota_retry_request_t;



/// @brief Message signaling the end of the update.
typedef struct __attribute__((packed)) {
	uint8_t _dummy;
} spv_ota_end_t;


/// @brief Sends an OTA request message.
///
/// @param[in] handle Mesh handle.
/// @param[in] message OTA request message.
///
/// @return `ESP_OK` or error.
esp_err_t spv_ota_send_request(
	wmesh_handle_t *handle,
	const wmesh_address_t gateway_address
);


/// @brief Sends an OTA begin message.
///
/// @param[in] handle Mesh handle.
/// @param[in] message OTA begin message.
///
/// @return `ESP_OK` or error.
esp_err_t spv_ota_send_begin(
	wmesh_handle_t *handle,
	const spv_ota_begin_t *message
);


/// @brief Sends an OTA data message.
///
/// @param[in] handle Mesh handle.
/// @param[in] data OTA data message.
///
/// @return `ESP_OK` or error.
esp_err_t spv_ota_send_data(
	wmesh_handle_t *handle,
	const spv_ota_data_t *data
);


/// @brief Requests the Gateway to restart the OTA update.
///
/// @param[in] handle Mesh handle.
/// @param[in] gateway_address Gateway mesh address.
/// @param[in] data OTA retry message.
///
/// @return `ESP_OK` or error.
esp_err_t spv_ota_send_retry_request(
	wmesh_handle_t *handle,
	const wmesh_address_t gateway_address,
	const spv_ota_retry_request_t *data
);


/// @brief Sends an OTA end message.
///
/// @param[in] handle Mesh handle.
/// @param[in] message OTA end message.
///
/// @return `ESP_OK` or error.
esp_err_t spv_ota_send_end(
	wmesh_handle_t *handle
);


/// @brief Type of received Gateway message.
typedef enum {

	OTA_TYPE_FIRST,

	/// @brief Error when decoding the message.
	OTA_TYPE_ERROR,

	OTA_TYPE_REQUEST,

	OTA_TYPE_BEGIN,

	OTA_TYPE_DATA,

	OTA_TYPE_RETRY,

	OTA_TYPE_END,

	OTA_TYPE_LAST,

} spv_ota_message_type_t;


/// @brief Returned when an OTA message is received.
typedef struct {

	/// @brief Indicates which type of message was received.
	spv_ota_message_type_t type;

	union {

		/// @brief Update request message. Only valid when `type` is
		/// `OTA_TYPE_REQUEST`.
		spv_ota_request_t *request;

		/// @brief Update begin message. Only valid when `type` is
		/// `OTA_TYPE_BEGIN`.
		spv_ota_begin_t *begin;

		/// @brief OTA data message. Only valid when `type` is
		/// `OTA_TYPE_DATA`.
		spv_ota_data_t *data;

		/// @brief OTA retry message. Only valid when `type` is
		/// `OTA_TYPE_RETRY`.
		spv_ota_retry_request_t *retry;

		/// @brief OTA end message. Only valid when `type` is
		/// `OTA_TYPE_END`.
		spv_ota_end_t *end;
	};

} spv_ota_received_message_t;


/// @brief Decodes an OTA message.
///
/// @attention Return value contains a pointer to `data`. As such, its lifetime is
/// the same as `data`'s.
///
/// @param data Raw data.
/// @param data_size Data size in bytes.
///
/// @return A struct with the decoded message. Only valid if the `type` field is
/// not `OTA_TYPE_ERROR`.
spv_ota_received_message_t spv_ota_decode_message(
	uint8_t *data,
	size_t data_size
);

#endif

#ifndef SPV_SERVICES_GATEWAY_H_
#define SPV_SERVICES_GATEWAY_H_

#include "sdkconfig.h"

#include <stdint.h>
#include "time/clock.h"
#include "wmesh/wmesh.h"


/// @brief Used to tell nodes which Wi-Fi channel to use.
typedef struct __attribute__((packed)) {

	/// @brief 2.4GHz Wi-Fi channel.
	uint8_t channel;

} spv_gateway_channel_advertisement_t;


/// @brief Gateway announcement. Sent when a gateway starts up.
typedef struct __attribute__((packed)) {

	/// @brief Unix time in seconds.
	spv_timestamp_t current_time;

	/// @brief Gateway firmware version in use.
	uint64_t firmware_version;

	/// @brief Gateway status flags.
	struct __attribute__((packed)) {

		/// @brief True if the gateway has internet connectivity. False otherwise.
		bool has_connectivity:1;

	} flags;

} spv_gateway_advertisement_t;


/// @brief Sent by the gateway to start a sleep period.
typedef struct __attribute__((packed)) {

	/// @brief Next time to wake.
	spv_timestamp_t wake_time;

} spv_gateway_sleep_command_t;



/// @brief Send a channel advertisement.
///
/// @param[in] handle Mesh handle.
/// @param[in] advertisement Advertisement data.
///
/// @return `ESP_OK` or error.
esp_err_t spv_gateway_send_channel(
	wmesh_handle_t *handle,
	const spv_gateway_channel_advertisement_t *advertisement
);


/// @brief Advertises this node as a gateway.
///
/// @param[in] handle Mesh handle.
/// @param[in] announcement Advertisement to send.
///
/// @return `ESP_OK` or error.
esp_err_t spv_gateway_send_advertisement(
	wmesh_handle_t *handle,
	const spv_gateway_advertisement_t *advertisement
);


/// @brief Sends a sleep message to the network.
///
/// @param[in] handle Mesh handle.
/// @param[in] sleep_command Sleep command.
///
/// @return `ESP_OK` or error.
esp_err_t spv_gateway_send_sleep(
	wmesh_handle_t *handle,
	const spv_gateway_sleep_command_t *sleep_command
);


/// @brief Type of received Gateway message.
typedef enum {

	/// @brief Error when decoding the message.
	GATEWAY_TYPE_ERROR,

	GATEWAY_TYPE_CHANNEL,

	GATEWAY_TYPE_ADVERTISEMENT,

	GATEWAY_TYPE_SLEEP,

} spv_gateway_message_type_t;


/// @brief Returned when a Gateway message is received.
typedef struct {

	/// @brief Indicates which type of message was received.
	spv_gateway_message_type_t type;

	union {

		/// @brief Channel advertisement. Only valid when `type` is
		/// `GATEWAY_TYPE_CHANNEL`.
		spv_gateway_channel_advertisement_t *channel;

		/// @brief Gateway advertisement message. Only valid when `type` is
		/// `GATEWAY_TYPE_ADVERTISEMENT`.
		spv_gateway_advertisement_t *advertisement;

		/// @brief Sleep message. Only valid when `type` is
		/// `GATEWAY_TYPE_SLEEP`.
		spv_gateway_sleep_command_t *sleep_command;

	};

} spv_gateway_received_message_t;


/// @brief Decodes a Gateway message.
///
/// @attention Return value contains a pointer to `data`. As such, its lifetime is
/// the same as `data`'s.
///
/// @param data Raw data.
/// @param data_size Data size in bytes.
///
/// @return A struct with the decoded message. Only valid if the `type` field is
/// not `GATEWAY_TYPE_ERROR`.
spv_gateway_received_message_t spv_gateway_decode_message(
	uint8_t *data,
	size_t data_size
);

#endif

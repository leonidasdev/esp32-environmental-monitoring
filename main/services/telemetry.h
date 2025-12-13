#ifndef SPV_SERVICES_TELEMETRY_H_
#define SPV_SERVICES_TELEMETRY_H_

#include "sdkconfig.h"

#include <stdint.h>
#include "sensors/ulp.h"
#include "wmesh/wmesh.h"

#include "gateway.h"

typedef struct {
	uint16_t noise, luminosity, co2, voc, humidity, temperature;
} spv_telemetry_reading_t;

typedef struct __attribute__((packed)){
	spv_timestamp_t fecha;

	char node_name[CONFIG_SPV_TELEMETRY_NODE_NAME_LENGTH];

	uint16_t num_datos;

	spv_telemetry_reading_t datos[];

}spv_telemetry_msg;


esp_err_t spv_telemetry_send_msg(
    wmesh_handle_t *handle,
    const spv_telemetry_msg *msg,
    const wmesh_address_t gateway_address
);

/// @brief Type of received TELEMETRY message.
typedef enum {

	/// @brief Error when decoding the message.
	TELEMETRY_TYPE_ERROR,

	TELEMETRY_TYPE_MSG,

} spv_telemetry_message_type_t;


/// @brief Returned when a TELEMETRY message is received.
typedef struct {

	/// @brief Indicates which type of message was received.
	spv_telemetry_message_type_t type;

	union {

		/// @brief Message. Only valid when `type` is
		/// `TELEMETRY_TYPE_MSG`.
		spv_telemetry_msg *msg;

	};

} spv_telemetry_received_message_t;


spv_telemetry_received_message_t spv_telemetry_decode_message(
    uint8_t *data,
    size_t data_size
);

#endif

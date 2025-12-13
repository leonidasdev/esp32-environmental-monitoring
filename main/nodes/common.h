#ifndef SPV_NODES_COMMON_H_
#define SPV_NODES_COMMON_H_

#include "cJSON.h"
#include "services/telemetry.h"
#include "time/clock.h"

/// @brief Converts a mesh telemetry message into Thingsboard JSON.
///
/// @param[in] msg Telemetry message.
///
/// @return JSON object or `NULL`.
cJSON* spv_telemetry_msg_to_json(spv_telemetry_msg *msg);


/// @brief Converts a telemetry reading into Thingsboard JSON.
///
/// @param[in] msg Telemetry message.
/// @param[in] node_name Node name.
/// @param[in] timestamp Reading timestamp.
///
/// @return JSON object or `NULL`.
cJSON* spv_telemetry_reading_to_json(
	spv_telemetry_reading_t *reading,
	const char *node_name,
	spv_timestamp_t timestamp
);




#endif

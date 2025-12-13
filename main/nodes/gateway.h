#ifndef SPV_NODES_GATEWAY_H_
#define SPV_NODES_GATEWAY_H_

#include "config/persistence.h"

/// @brief Starts this device as a gateway.
///
/// @param[in] config Device configuration.
void spv_start_gateway(
	const spv_config_t *config
);

#endif

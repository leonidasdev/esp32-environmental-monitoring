#ifndef SPV_NODES_NODE_H_
#define SPV_NODES_NODE_H_

#include "config/persistence.h"


/// @brief Starts this device as a node.
///
/// @param[in] config Device configuration.
void spv_start_node(
	const spv_config_t *config
);

#endif

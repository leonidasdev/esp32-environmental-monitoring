#ifndef WMESH_COMMON_H_
#define WMESH_COMMON_H_

#include <stdint.h>

struct wmesh_handle_t;

/// @brief Mesh node address.
typedef uint8_t wmesh_address_t[6];

/// @brief Any message sent to this address will be received by all nodes.
static const wmesh_address_t wmesh_broadcast_address = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#endif

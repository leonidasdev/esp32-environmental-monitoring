#ifndef SPV_CONFIG_PROVISIONING_H_
#define SPV_CONFIG_PROVISIONING_H_

#include "esp_err.h"


/// @brief Starts the provisioning process.
///
/// @return `ESP_OK` or error.
esp_err_t spv_provisioning_start();

#endif

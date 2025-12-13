#ifndef SPV_SENSORS_SENSORS_H_
#define SPV_SENSORS_SENSORS_H_

#include "esp_err.h"

/// @brief Initializes all sensors. Must be called on boot, but not after waking
/// from sleep.
///
/// @return `ESP_OK` or error.
esp_err_t spv_sensors_init();

#endif

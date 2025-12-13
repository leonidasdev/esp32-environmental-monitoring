#ifndef SPV_SENSORS_ULP_H_
#define SPV_SENSORS_ULP_H_

#include "esp_err.h"
#include "sdkconfig.h"
#include "time/clock.h"

/// @brief Initializes and starts the ULP.
///
/// @return `ESP_OK` or error.
esp_err_t spv_ulp_start();


/// @brief Returns the time at which the ULP was started.
spv_timestamp_t spv_ulp_start_time_get();


/// @brief Used to indicate the type of reading to fetch.
typedef enum {
	SPV_ULP_READING_FIRST,

	SPV_ULP_NOISE_READING,
	SPV_ULP_LUMINOSITY_READING,

	SPV_ULP_CO2_READING,
	SPV_ULP_VOC_READING,

	SPV_ULP_HUMIDITY_READING,
	SPV_ULP_TEMPERATURE_READING,

	SPV_ULP_READING_LAST
} spv_ulp_reading_type_t;


/// @brief Returns how many readings of a given type are stored in RTC memory.
///
/// @param[in] type Type of reading.
///
/// @return Number of readings available.
size_t spv_ulp_get_reading_count(spv_ulp_reading_type_t type);


/// @brief Returns a raw reading from the ULP.
///
/// @param[in] index Index of the value to read.
/// @param[in] type Type of reading.
///
/// @return A 16-bit result representing the stored value.
uint16_t spv_ulp_get_reading(size_t index, spv_ulp_reading_type_t type);


/// @brief Returns a converted reading from the ULP.
///
/// @param[in] index Index of the value to read.
/// @param[in] type Type of reading.
///
/// @return A 16-bit result of the converted value.
uint16_t spv_ulp_get_reading_cvt(size_t index, spv_ulp_reading_type_t type);


#endif

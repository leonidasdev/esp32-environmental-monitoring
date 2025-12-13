#ifndef SPV_TIME_CLOCK_H_
#define SPV_TIME_CLOCK_H_

#include <stdint.h>
#include "esp_err.h"
#include "sdkconfig.h"

#if CONFIG_SPV_TIMESTAMP_SIZE == 8
	/// @brief Unix timestamp in seconds.
	typedef uint64_t spv_timestamp_t;

#elif CONFIG_SPV_TIMESTAMP_SIZE == 4
	/// @brief Unix timestamp in seconds.
	typedef uint32_t spv_timestamp_t;

#else
	#error Invalid Gateway timestamp size
#endif


/// @brief Sets the current time.
///
/// @param[in] time UNIX timestamp in seconds.
void time_set(spv_timestamp_t time);


/// @brief Syncs the system's time with an NTP server.
///
/// @note The system must have Internet connectivity for this function to work.
///
/// @return `ESP_OK` or error.
esp_err_t time_ntp_sync();


/// @brief Returns the current time.
///
/// @return Current UNIX time in seconds.
spv_timestamp_t time_get();


/// @brief Returns the number of seconds since boot.
///
/// @return Time since boot in seconds.
spv_timestamp_t time_since_boot();


/// @brief Returns the date at which the system booted up.
///
/// @return Timestamp in seconds.
spv_timestamp_t time_get_boot();

#endif

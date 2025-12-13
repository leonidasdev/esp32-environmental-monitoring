#include "clock.h"

#include <time.h>
#include "esp_log.h"
#include "esp_sntp.h"

static const char *TAG = "SPV Time";


void time_set(spv_timestamp_t time) {
	ESP_LOGI(TAG, "System time updated");

	struct timeval current_time = {
		.tv_sec = time,
		.tv_usec = 0
	};
	struct timezone timezone = {
		.tz_minuteswest = 0,
		.tz_dsttime = 0
	};
	settimeofday(&current_time, &timezone);
}


esp_err_t time_ntp_sync() {
	return ESP_FAIL;
}


spv_timestamp_t time_get() {
	struct timeval current_time = { 0 };
	struct timezone timezone = {
		.tz_minuteswest = 0,
		.tz_dsttime = 0
	};
	gettimeofday(&current_time, &timezone);

	return current_time.tv_sec;
}


spv_timestamp_t time_since_boot() {
	return pdTICKS_TO_MS(xTaskGetTickCount()) / 1000;
}


spv_timestamp_t time_get_boot() {
	return time_since_boot() + time_get();
}

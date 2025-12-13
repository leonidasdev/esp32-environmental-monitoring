#include "sensors/ulp.h"

#include "esp_log.h"
#include "driver/rtc_io.h"
#include "soc/rtc_periph.h"
#include "ulp.h"
#include "ulp_common.h"
#include "ulp_fsm_common.h"
#include "ulp_sensors.h"
#include "time/clock.h"

static const char *TAG = "SPV ULP";
RTC_DATA_ATTR spv_timestamp_t spv_ulp_start_time;

static const uint32_t *volatile value_counts[] = {
	[SPV_ULP_NOISE_READING] = &ulp_microphone_count,
	[SPV_ULP_LUMINOSITY_READING] = &ulp_luminosity_count,

	[SPV_ULP_CO2_READING] = &ulp_gas_count,
	[SPV_ULP_VOC_READING] = &ulp_gas_count,

	[SPV_ULP_HUMIDITY_READING] = &ulp_env_count,
	[SPV_ULP_TEMPERATURE_READING] = &ulp_env_count,
};
static const uint32_t *volatile value_ptrs[] = {
	[SPV_ULP_NOISE_READING] = &ulp_microphone_values,
	[SPV_ULP_LUMINOSITY_READING] = &ulp_luminosity_values,

	[SPV_ULP_CO2_READING] = &ulp_gas_values,
	[SPV_ULP_VOC_READING] = &ulp_gas_values,

	[SPV_ULP_HUMIDITY_READING] = &ulp_env_values,
	[SPV_ULP_TEMPERATURE_READING] = &ulp_env_values,
};
static const size_t value_sizes[] = {
	[SPV_ULP_NOISE_READING] = 1,
	[SPV_ULP_LUMINOSITY_READING] = 1,

	[SPV_ULP_CO2_READING] = 2,
	[SPV_ULP_VOC_READING] = 2,

	[SPV_ULP_HUMIDITY_READING] = 2,
	[SPV_ULP_TEMPERATURE_READING] = 2,
};
static const size_t value_offsets[] = {
	[SPV_ULP_NOISE_READING] = 0,
	[SPV_ULP_LUMINOSITY_READING] = 0,

	[SPV_ULP_CO2_READING] = 0,
	[SPV_ULP_VOC_READING] = 1,

	[SPV_ULP_HUMIDITY_READING] = 0,
	[SPV_ULP_TEMPERATURE_READING] = 1,
};


extern const uint8_t ulp_bin_start[] asm("_binary_ulp_sensors_bin_start");
extern const uint8_t ulp_bin_end[]   asm("_binary_ulp_sensors_bin_end");
esp_err_t spv_ulp_start() {
	// I²C pins
	rtc_gpio_init(CONFIG_SPV_SENSOR_I2C_SDA_GPIO);
	rtc_gpio_init(CONFIG_SPV_SENSOR_I2C_SCL_GPIO);

	rtc_gpio_pullup_en(CONFIG_SPV_SENSOR_I2C_SDA_GPIO);
	rtc_gpio_pullup_en(CONFIG_SPV_SENSOR_I2C_SCL_GPIO);

	rtc_gpio_set_direction(CONFIG_SPV_SENSOR_I2C_SDA_GPIO, RTC_GPIO_MODE_INPUT_ONLY);
	rtc_gpio_set_direction(CONFIG_SPV_SENSOR_I2C_SCL_GPIO, RTC_GPIO_MODE_INPUT_ONLY);

	// SPI pins
	rtc_gpio_init(CONFIG_SPV_SENSOR_SPI_MISO_GPIO);
	rtc_gpio_init(CONFIG_SPV_SENSOR_SPI_SCK_GPIO);
	rtc_gpio_init(CONFIG_SPV_SENSOR_MICROPHONE_CS_GPIO);
	rtc_gpio_init(CONFIG_SPV_SENSOR_LUMINOSITY_CS_GPIO);

	rtc_gpio_set_direction(CONFIG_SPV_SENSOR_SPI_MISO_GPIO, RTC_GPIO_MODE_INPUT_ONLY);
	rtc_gpio_set_direction(CONFIG_SPV_SENSOR_SPI_SCK_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
	rtc_gpio_set_direction(CONFIG_SPV_SENSOR_MICROPHONE_CS_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
	rtc_gpio_set_direction(CONFIG_SPV_SENSOR_LUMINOSITY_CS_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);

	rtc_gpio_set_level(CONFIG_SPV_SENSOR_SPI_SCK_GPIO, 1);
	rtc_gpio_set_level(CONFIG_SPV_SENSOR_MICROPHONE_CS_GPIO, 1);
	rtc_gpio_set_level(CONFIG_SPV_SENSOR_LUMINOSITY_CS_GPIO, 1);

	esp_err_t err = ulp_load_binary(0, ulp_bin_start, (ulp_bin_end - ulp_bin_start) / sizeof(uint32_t));
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error loading ULP binary into RTC memory: %s", esp_err_to_name(err));
		return err;
	}

	for(spv_ulp_reading_type_t type = SPV_ULP_READING_FIRST + 1; type < SPV_ULP_READING_LAST; type++) {
		value_counts[type] = 0;
	}

	ulp_set_wakeup_period(0, 1000 * 1000 * CONFIG_SPV_SENSOR_READ_FREQUENCY_SECONDS);
	err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error starting ULP: %s", esp_err_to_name(err));
		return err;
	}
	spv_ulp_start_time = time_get();

	return ESP_OK;
}


spv_timestamp_t spv_ulp_start_time_get() {
	return spv_ulp_start_time;
}


size_t spv_ulp_get_reading_count(spv_ulp_reading_type_t type) {
	uint32_t count = *value_counts[type];
	count &= 0x0000FFFF;
	count /= value_sizes[type];
	return (size_t) count - 1;
}


uint16_t spv_ulp_get_reading(size_t index, spv_ulp_reading_type_t type) {
	uint32_t value =
		value_ptrs[type][index * value_sizes[type]] + value_offsets[type];
	value &= 0x0000FFFF;
	return (uint16_t) value;
}


static const float conversion[] = {
	[SPV_ULP_NOISE_READING] = 0.01239476145,
	[SPV_ULP_LUMINOSITY_READING] = 1.0,

	[SPV_ULP_CO2_READING] = 1.0,
	[SPV_ULP_VOC_READING] = 1.0,

	[SPV_ULP_HUMIDITY_READING] = 1.0/65536.0 * 100.0,

	// NO CONVERSION CONSTANT
	//[SPV_ULP_TEMPERATURE_READING] = 1.0,
};
uint16_t spv_ulp_get_reading_cvt(size_t index, spv_ulp_reading_type_t type) {

	uint16_t value = spv_ulp_get_reading(index, type);
	if(conversion[type] == 1.0) {
		return value;
	}

	if(type == SPV_ULP_TEMPERATURE_READING) {
		return value * 1.0/65536.0 * 165.0 - 40.0;
	} else {
		return (float) value * conversion[type];
	}
}

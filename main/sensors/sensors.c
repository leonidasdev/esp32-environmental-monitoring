#include "sensors/sensors.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"

static const char *TAG = "SPV Sensors";

static esp_err_t iaq_init(i2c_master_bus_handle_t bus_handle);
static esp_err_t humidity_init(i2c_master_bus_handle_t bus_handle);


esp_err_t spv_sensors_init() {
	i2c_master_bus_config_t i2c_config = {
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.i2c_port = 0,
		.scl_io_num = CONFIG_SPV_SENSOR_I2C_SCL_GPIO,
		.sda_io_num = CONFIG_SPV_SENSOR_I2C_SDA_GPIO,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
	};

	esp_err_t err;
	i2c_master_bus_handle_t bus_handle;
	if((err = i2c_new_master_bus(&i2c_config, &bus_handle)) != ESP_OK) {
		ESP_LOGE(TAG, "Error initializing I2C interface: %s", esp_err_to_name(err));
		return err;
	}

	if((err = iaq_init(bus_handle)) != ESP_OK) {
		ESP_LOGE(TAG, "Error initializing IAQ sensor");
		ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
		return err;
	}

	if((err = humidity_init(bus_handle)) != ESP_OK) {
		ESP_LOGE(TAG, "Error initializing humidity sensor");
		ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
		return err;
	}

	ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
	ESP_LOGI(TAG, "Successfully configured all sensors");
	return ESP_OK;
}


static uint8_t iaq_measurement_mode_register = 0x01;
static uint8_t iaq_app_start_register = 0xF4;
static uint8_t iaq_sw_reset_register = 0xFF;
static uint8_t iaq_reset_magic[] = { 0x11, 0xE5, 0x72, 0x8A };
enum iaq_config {
	IAQ_MODE_IDLE = 0,
	IAQ_MODE_1_SECOND = 0b00010000,
	IAQ_MODE_10_SECONDS = 0b00100000,
	IAQ_MODE_60_SECONDS = 0b00110000,
	IAQ_MODE_250_MILLISECONDS = 0b01000000,
};
static esp_err_t iaq_init(i2c_master_bus_handle_t bus_handle) {
	i2c_device_config_t iaq_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = CONFIG_SPV_SENSOR_GAS_I2C_ADDRESS,
		.scl_speed_hz = 100000,
	};


	esp_err_t err;
	i2c_master_dev_handle_t iaq_handle;
	if((err = i2c_master_bus_add_device(bus_handle, &iaq_cfg, &iaq_handle)) != ESP_OK) {
		ESP_LOGE(TAG, "Error adding IAQ sensor as slave: %s", esp_err_to_name(err));
		return err;
	}

	// Initialize IAQ sensor.
	err = i2c_master_transmit(
		iaq_handle,
		(uint8_t []) {
			iaq_sw_reset_register,
			iaq_reset_magic[0],
			iaq_reset_magic[1],
			iaq_reset_magic[2],
			iaq_reset_magic[3],
		},
		5,
		100
	);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error resetting IAQ sensor: %s", esp_err_to_name(err));
		i2c_master_bus_rm_device(iaq_handle);
		return err;
	}

	vTaskDelay(pdMS_TO_TICKS(20));

	err = i2c_master_transmit(
		iaq_handle,
		(uint8_t []) {
			iaq_app_start_register,
		},
		1,
		100
	);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error selecting IAQ register: %s", esp_err_to_name(err));
		i2c_master_bus_rm_device(iaq_handle);
		return err;
	}

	vTaskDelay(pdMS_TO_TICKS(1));

	err = i2c_master_transmit(
		iaq_handle,
		(uint8_t []) {
			iaq_measurement_mode_register,
			IAQ_MODE_1_SECOND
		},
		2,
		100
	);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error sending configuration data to IAQ sensor: %s", esp_err_to_name(err));
	} else {
		ESP_LOGI(TAG, "Successfully configured IAQ sensor");
	}

	i2c_master_bus_rm_device(iaq_handle);
	return err;
}


static uint8_t hdc1080_config_register = 0x02;
enum hcd1080_config {
	HDC1080_ENABLE_TEMPERATURE_HUMIDITY = 0b00010000,

	HDC1080_TEMPERATURE_14_BIT = 0b00000000,
	HDC1080_TEMPERATURE_11_BIT = 0b00000100,

	HDC1080_HUMIDITY_14_BIT = 0b00000000,
	HDC1080_HUMIDITY_11_BIT = 0b00000001,
	HDC1080_HUMIDITY_8_BIT = 0b00000010,
};
static esp_err_t humidity_init(i2c_master_bus_handle_t bus_handle) {
	i2c_device_config_t hygro_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = CONFIG_SPV_SENSOR_ENV_I2C_ADDRESS,
		.scl_speed_hz = 100000,
	};

	esp_err_t err;
	i2c_master_dev_handle_t hygro_handle;
	if((err = i2c_master_bus_add_device(bus_handle, &hygro_cfg, &hygro_handle)) != ESP_OK) {
		ESP_LOGE(TAG, "Error adding humidity sensor as slave: %s", esp_err_to_name(err));
		return err;
	}

	err = i2c_master_transmit(
		hygro_handle,
		(uint8_t []) {
			hdc1080_config_register,

			HDC1080_ENABLE_TEMPERATURE_HUMIDITY |
			HDC1080_TEMPERATURE_11_BIT |
			HDC1080_HUMIDITY_8_BIT,

			0x00
		},
		3,
		100
	);
	if(err != ESP_OK) {
		ESP_LOGE(TAG, "Error configuring humidity sensor: %s", esp_err_to_name(err));
	} else {
		ESP_LOGI(TAG, "Successfully configured humidity sensor");
	}

	i2c_master_bus_rm_device(hygro_handle);
	return err;
}

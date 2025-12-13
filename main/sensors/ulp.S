#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "soc/sens_reg.h"
#include "sdkconfig.h"

.altmacro
#include "i2c.S"
#include "spi.S"

#define MICROPHONE_CS CONFIG_SPV_SENSOR_MICROPHONE_CS_RTC	// Pmod MIC3
#define LUMINOSITY_CS CONFIG_SPV_SENSOR_LUMINOSITY_CS_RTC	// Pmod ALS

#define GAS_SENSOR_ADDRESS CONFIG_SPV_SENSOR_GAS_I2C_ADDRESS	// Pmod AQS
#define ENV_SENSOR_ADDRESS CONFIG_SPV_SENSOR_ENV_I2C_ADDRESS	// HDC1080

#define TOTAL_READINGS CONFIG_SPV_SENSOR_READ_BUFFER_COUNT


// Stores the contents of r0 in `values + count`.
// Clobbers:
//	- r1
//	- r2
.macro STORE_WORD values, count
		MOVE r1, values
		MOVE r2, count
		LD r2, r2, 0
		ADD r2, r1, r2
		ST r0, r2, 0
.endm

// Increments a counter.
// Clobbers:
//	- r1
//	- r2
.macro INC_COUNTER count
	MOVE r2, count
	LD r1, r2, 0
	ADD r1, r1, 1
	ST r1, r2, 0
.endm


.bss
.global microphone_count
microphone_count:
	.long 0

.global microphone_values
microphone_values:
	.space (TOTAL_READINGS * 4)	// 2 Bytes per reading


.global luminosity_count
luminosity_count:
	.long 0

.global luminosity_values
luminosity_values:
	.space (TOTAL_READINGS * 4) // 1 Byte per reading


.global gas_count
gas_count:
	.long 0

.global gas_values
gas_values:
	.space (TOTAL_READINGS * 4) // 4 Bytes per reading, 2 for CO², 2 for VOC


.global env_count
env_count:
	.long 0

.global env_values
env_values:
	.space (TOTAL_READINGS * 4)	// 3 Bytes per reading, 1 for humidity, 2 for temperature


.text
.global entry
entry:
	WAIT 60

	// Start a humidity and temperature conversion early.
	.env_conversion_start:
		// Write 0x00 to the sensor.
		I2C_START

		MOVE r1, (ENV_SENSOR_ADDRESS << 1) | 0
		I2C_WRITE_BYTE r1
		I2C_READ_ACK

		MOVE r1, 0x00
		I2C_WRITE_BYTE r1
		I2C_READ_ACK

		I2C_STOP
	.env_conversion_end:


	.microphone_start:
		SPI_READ 16, MICROPHONE_CS
		STORE_WORD microphone_values, microphone_count
		INC_COUNTER microphone_count
	.microphone_end:


	.luminosity_start:
		// Total of 15 bits: 3 padding + 8 data + 4 padding
		SPI_READ 15, LUMINOSITY_CS
		// Remove right side padding.
		RSH r0, r0, 4
		STORE_WORD luminosity_values, luminosity_count
		INC_COUNTER luminosity_count
	.luminosity_end:


	.gas_start:
		// To read the ALG_RESULT_DATA register:
		//	Write 0x02
		//	Read 4 bytes:
		//		First 2: CO²
		//		Last 2: VOC

		I2C_START

		MOVE r1, (GAS_SENSOR_ADDRESS << 1) | 0
		I2C_WRITE_BYTE r1
		I2C_READ_ACK

		MOVE r1, 0x02	// ALG_RESULT_DATA
		I2C_WRITE_BYTE r1
		I2C_READ_ACK

		I2C_STOP


		I2C_START

		MOVE r1, (GAS_SENSOR_ADDRESS << 1) | 1
		I2C_WRITE_BYTE r1
		I2C_READ_ACK

		MOVE r1, 0
		I2C_READ_BYTE r1
		I2C_WRITE_ACK 0
		I2C_READ_BYTE r1
		I2C_WRITE_ACK 0

		MOVE r0, r1
		STORE_WORD gas_values, gas_count
		INC_COUNTER gas_count

		MOVE r1, 0
		I2C_READ_BYTE r1
		I2C_WRITE_ACK 0
		I2C_READ_BYTE r1
		I2C_WRITE_ACK 1

		MOVE r0, r1
		STORE_WORD gas_values, gas_count
		INC_COUNTER gas_count

		I2C_STOP
	.gas_end:


	.env_start:
		// The conversion duration depends on the configured precision.
		// For 8-bit humidity and 11 bit temperature:
		//	8-bit Humidity: 2.50ms
		//	11-bit Temperature: 3.65ms
		//		Total: 6.15ms
		//
		// Since the ULP runs at 8MHz, we need to wait at least 49200 cycles.
		WAIT 49200

		I2C_START

		MOVE r1, (ENV_SENSOR_ADDRESS << 1) | 1
		I2C_WRITE_BYTE r1
		I2C_READ_ACK

		MOVE r1, 0
		I2C_READ_BYTE r1
		I2C_WRITE_ACK 0
		I2C_READ_BYTE r1
		I2C_WRITE_ACK 0

		MOVE r0, r1
		STORE_WORD env_values, env_count
		INC_COUNTER env_count

		MOVE r1, 0
		I2C_READ_BYTE r1
		I2C_WRITE_ACK 0
		I2C_READ_BYTE r1
		I2C_WRITE_ACK 1

		MOVE r0, r1
		STORE_WORD env_values, env_count
		INC_COUNTER env_count

		I2C_STOP
	.env_stop:


	HALT

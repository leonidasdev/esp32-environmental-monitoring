#include "sdkconfig.h"

#define MISO_RTC_PIN CONFIG_SPV_SENSOR_SPI_MISO_RTC
#define SCK_RTC_PIN CONFIG_SPV_SENSOR_SPI_SCK_RTC

#define SPI_WAIT_CYCLES CONFIG_SPV_ULP_SPI_WAIT
#define SPI_WAIT WAIT SPI_WAIT_CYCLES
#define SPI_WAIT_HALF WAIT SPI_WAIT_CYCLES / 2

#define SCK_SET WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S+SCK_RTC_PIN, 1, 1)
#define SCK_CLEAR WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S+SCK_RTC_PIN, 1, 0)

.macro CS_SET pin
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S+pin, 1, 1)
.endm

.macro CS_CLEAR pin
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S+pin, 1, 0)
.endm

// Stores in r0 the value in MOSI.
.macro MISO_READ
	READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S+MISO_RTC_PIN, 1)
.endm


// Reads n bits from the SPI bus and stores them in r0
// Clobbers:
//	- Stage counter.
//	- r1
.macro SPI_READ bits, cs
	LOCAL .loop

	STAGE_RST
	MOVE r1, 0

	CS_CLEAR cs
	SPI_WAIT

	.loop:
		SCK_CLEAR
		SPI_WAIT

		SCK_SET
		SPI_WAIT_HALF

		MISO_READ
		LSH r1, r1, 1
		OR r1, r1, r0
		SPI_WAIT_HALF

		STAGE_INC 1
		JUMPS .loop, bits, LT

	MOVE r0, r1

	CS_SET cs
.endm

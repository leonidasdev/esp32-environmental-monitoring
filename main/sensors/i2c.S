
#include "sdkconfig.h"

#define SDA_RTC_PIN CONFIG_SPV_SENSOR_I2C_SDA_RTC
#define SCL_RTC_PIN CONFIG_SPV_SENSOR_I2C_SCL_RTC

#define I2C_WAIT_CYCLES CONFIG_SPV_ULP_I2C_WAIT

.macro I2C_WAIT
	WAIT I2C_WAIT_CYCLES
.endm

.macro I2C_WAIT_HALF
	WAIT I2C_WAIT_CYCLES / 2
.endm


// Stores in r0 the value in the I²C bus.
.macro SDA_GET
	READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + SDA_RTC_PIN, 1)
.endm

// Stores in r0 whether SCL is high or low.
.macro SCL_GET
	READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + SCL_RTC_PIN, 1)
.endm

.macro SDA_SET
	WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TC_REG, RTC_GPIO_ENABLE_W1TC_S+SDA_RTC_PIN, 1, 1)
.endm

.macro SDA_CLEAR
	WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TS_REG, RTC_GPIO_ENABLE_W1TS_S+SDA_RTC_PIN, 1, 1)
.endm

// Clobbers r0
.macro SCL_SET
	LOCAL .wait_rise
	WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TC_REG, RTC_GPIO_ENABLE_W1TC_S+SCL_RTC_PIN, 1, 1)

	.wait_rise:
		SCL_GET
		JUMPR .wait_rise, 0, EQ
.endm

.macro SCL_CLEAR
	WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TS_REG, RTC_GPIO_ENABLE_W1TS_S+SCL_RTC_PIN, 1, 1)
.endm


// Writes a START condition to the I²C bus.
.macro I2C_START
	SDA_CLEAR
	I2C_WAIT

	SCL_CLEAR
	I2C_WAIT
.endm

// Writes a STOP condition to the I²C bus.
.macro I2C_STOP
	SCL_CLEAR
	SDA_CLEAR
	I2C_WAIT

	SCL_SET
	I2C_WAIT

	SDA_SET
	I2C_WAIT
.endm

// Stores a 0 in the R0 register if a NACK was received, or different in case
// of an ACK.
.macro I2C_READ_ACK
	SCL_CLEAR
	I2C_WAIT_HALF

	SDA_SET
	I2C_WAIT_HALF

	SCL_SET
	I2C_WAIT_HALF

	SDA_GET
	I2C_WAIT_HALF
.endm

// Writes a 1 or 0 to the I²C bus.
// Clobbers:
//	- R0
.macro I2C_WRITE_ACK ack
	LOCAL .send_ack, .send_nack, .send_end

	SCL_CLEAR
	I2C_WAIT_HALF

	.if ack == 1
		SDA_SET
	.else
		SDA_CLEAR
	.endif

	I2C_WAIT_HALF
	SCL_SET
	I2C_WAIT

	.if ack == 0
		SCL_CLEAR
		I2C_WAIT_HALF
		SDA_SET
	.endif

.endm

// Writes one byte to the bus stored in the `value_reg` register.
//
// Clobbers:
//	- Stage counter
//	- R0
.macro I2C_WRITE_BYTE value_reg
	LOCAL .loop, .send_one, .send_zero, .send_end
	STAGE_RST
	.loop:
		SCL_CLEAR
		I2C_WAIT_HALF

		MOVE r0, value_reg
		AND r0, r0, 128
		LSH r1, r1, 1

		JUMPR .send_one, 1, GE
		.send_zero:
		SDA_CLEAR
		JUMP .send_end

		.send_one:
		SDA_SET

		.send_end:
		I2C_WAIT_HALF
		SCL_SET
		I2C_WAIT


		STAGE_INC 1
		JUMPS .loop, 8, LT;
.endm

// Reads one byte from the bus and writes it to r0
// Up to 2 bytes of data can be read to a single register by using two
// consecutive `I2C_READ_BYTE` calls. Make sure to clear the output register
// before calling.
//
// Clobbers:
//	- Stage counter
//	- r0
//	- Output register
.macro I2C_READ_BYTE output_reg
	LOCAL .loop

	STAGE_RST
	.loop:
		SCL_CLEAR
		I2C_WAIT_HALF

		SCL_SET
		I2C_WAIT_HALF

		SDA_GET
		LSH output_reg, output_reg, 1
		OR output_reg, output_reg, r0

		I2C_WAIT_HALF
		STAGE_INC 1
		JUMPS .loop, 8, LT
.endm

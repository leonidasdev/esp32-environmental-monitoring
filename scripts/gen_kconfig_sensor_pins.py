#!/bin/env python3

# Generates `Kconfig.sensor_pins`
# Usage: python3 gen_kconfig_sensor_pins.py

import textwrap

CONFIG = {
	"filename": "main/Kconfig.sensor_pins",

	"descriptors": [
		{
			"prompt": "Microphone Chip Select",
			"help": "Pin to use as the Pmod MIC3 microphone's chip select.",
			"default": 13,

			"name": "SPV_SENSOR_MICROPHONE_CS"
		},
		{
			"prompt": "Luminosity sensor Chip Select",
			"help": "Pin to use as the Pmod ALS luminosity sensor's chip select.",
			"default": 14,

			"name": "SPV_SENSOR_LUMINOSITY_CS"
		},
		{
			"prompt": "SPI SCK pin",
			"help": "Pin to use as SCK.",
			"default": 25,

			"name": "SPV_SENSOR_SPI_SCK"
		},
		{
			"prompt": "SPI MISO pin",
			"help": "Pin to use as MISO.",
			"default": 26,

			"name": "SPV_SENSOR_SPI_MISO"
		},
		{
			"prompt": "I²C SDA pin",
			"help": "Pin to use as SDA.",
			"default": 33,

			"name": "SPV_SENSOR_I2C_SDA"
		},
		{
			"prompt": "I²C SCL pin",
			"help": "Pin to use as SCL.",
			"default": 32,

			"name": "SPV_SENSOR_I2C_SCL"
		},
	],

	# Only pins with a GPIO<-->RTC mapping are included.
	# Strapping pins are commented.
	"pins": [
		# Format: (GPIO, RTC)
		#(0, 11),
		#(2, 12),
		(4, 10),
		#(12, 15),
		(13, 14),
		(14, 16),
		#(15, 13),
		(25, 6),
		(26, 7),
		(27, 17),
		(32, 9),
		(33, 8),

		# The following pins are Input ONLY.
		#(34, 4),
		#(35, 5),
		#(36, 0),
		#(37, 1),
		#(38, 2),
		#(39, 3)
	]
}


def write_choice(descriptor, file):
	prefix = descriptor["name"]
	file.write(textwrap.dedent(
		f"""
		choice {prefix}_IO
			prompt "{descriptor['prompt']}"
			default {prefix}_IO_{descriptor['default']}
			help
				{descriptor['help']}
		"""
	))

	for pin in CONFIG["pins"]:
		gpio, rtc = pin
		file.write(textwrap.indent(textwrap.dedent(
			f"""
			config {prefix}_IO_{gpio}
				bool "GPIO {gpio} / RTC {rtc}"
			"""
		), "\t"))

	file.write(textwrap.dedent(
		f"""
		endchoice

		"""
	))


def write_gpio_mapping(descriptor, file):
	prefix = descriptor["name"]
	file.write(textwrap.dedent(
		f"""
		config {prefix}_GPIO
			int
		"""
	))

	for pin in CONFIG["pins"]:
		gpio, _ = pin
		file.write(textwrap.indent(textwrap.dedent(
			f"""default {gpio} if {prefix}_IO_{gpio}
			"""
		), "\t"))

	file.write("\n")


def write_rtc_mapping(descriptor, file):
	prefix = descriptor["name"]
	file.write(textwrap.dedent(
		f"""
		config {prefix}_RTC
			int
		"""
	))

	for pin in CONFIG["pins"]:
		gpio, rtc = pin
		file.write(textwrap.indent(textwrap.dedent(
			f"""default {rtc} if {prefix}_IO_{gpio}
			"""
		), "\t"))

	file.write("\n")


with open(CONFIG["filename"], "w") as file:
	file.write(textwrap.dedent(
		"""
		# ============================== #
		#      WARNING. DO NOT EDIT      #
		#								 #
		#  Automatically generated file  #
		# See gen_kconfig_sensor_pins.py #
		#								 #
		# ============================== #


		"""
	))
	for descriptor in CONFIG["descriptors"]:
		write_choice(descriptor, file)
		write_gpio_mapping(descriptor, file)
		write_rtc_mapping(descriptor, file)

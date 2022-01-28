## Overview
The core of the firmware is in `boot.c`. The drivers for the individual sensors are in `devXXX.c` for sensor `XXX`. For example,
`devADXL362.c` for the ADXL362 3-axis accelerometer. The section below briefly describes all the source files in this directory. 


## Source File Descriptions

##### `CMakeLists-Warp.txt`
This is the CMake configuration file that is used. Edit this to change the default size of the stack and heap.

##### `SEGGER_RTT.*`
This is the implementation of the SEGGER Real-Time Terminal interface. Do not modify.

##### `SEGGER_RTT_Conf.h`
Configuration file for SEGGER Real-Time Terminal interface. You can increase the size of `BUFFER_SIZE_UP` to reduce text in the menu being trimmed.

##### `SEGGER_RTT_printf.c`
Implementation of the SEGGER Real-Time Terminal interface formatted I/O routines. Do not modify.

##### `devAdafruitBLESPIFriend.*`
Driver for Adafruit BLE SPI Friend.

##### `devINA219.*`
Driver for INA219.

##### `devADXL362.*`
Driver for Analog devices ADXL362.

##### `devAMG8834.*`
Driver for AMG8834.

##### `devAS7262.*`
Driver for AS7262.

##### `devAS7263.*`
Driver for AS7263.

##### `devAS726x.h`
Header file with definitions used by both `devAS7262.*` and `devAS7263.*`.

##### `devBME680.*`
Driver for BME680.

##### `devBMX055.*`
Driver for BMX055.

##### `devCCS811.*`
Driver for CCS811.

##### `devHDC1000.*`
Driver forHDC1000 .

##### `devIS25WP128.*`
Driver for IS25WP128.

##### `devISL23415.*`
Driver for ISL23415.

##### `devL3GD20H.*`
Driver for L3GD20H.

##### `devLPS25H.*`
Driver for LPS25H.

##### `devMAG3110.*`
Driver for MAG3110.

##### `devMMA8451Q.*`
Driver for MMA8451Q.

##### `devPAN1326.*`
Driver for PAN1326.

##### `devSI4705.*`
Driver for SI4705.

##### `devSI7021.*`
Driver for SI7021.

##### `devTCS34725.*`
Driver for TCS34725.

##### `gpio_pins.c`
Definition of I/O pin configurations using the KSDK `gpio_output_pin_user_config_t` structure.

##### `gpio_pins.h`
Definition of I/O pin mappings and aliases for different I/O pins to symbolic names relevant to the Warp hardware design, via `GPIO_MAKE_PIN()`.

##### `startup_MKL03Z4.S`
Initialization assembler.

##### `boot.c`
The core of the implementation.

##### `powermodes.c`
Implements functionality related to enabling the different low-power modes of the KL03.

##### `warp.h`
Constant and data structure definitions.

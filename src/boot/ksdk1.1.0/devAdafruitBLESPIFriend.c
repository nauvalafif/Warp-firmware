//
// Created by Muhammad Rizqi Nauval Afif on 05/01/2022.
//

// Credit to Erich Styger https://github.com/ErichStyger/mcuoneclipse/tree/master/Examples/KDS/tinyK20/tinyK20_Adafruit_BLE

/*
 *	config.h needs to come first
 */
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"

#include "sdep.h"
#include "devAdafruitBLESPIFriend.h"

/* IRQ will be high to indicate data from the module */
/* CS is low active */

#define BLUEFRUIT_MODE_COMMAND   1
#define BLUEFRUIT_MODE_DATA      0

#define BLE_MAX_RESPONSE_SIZE  48
#define BLE_MAX_AT_CMD_SIZE    100

static uint8_t  _mode;
static uint8_t  m_tx_buffer[SDEP_MAX_PACKETSIZE];
static uint8_t  m_tx_count;

#define SPI_CS_ENABLE()   BLE_CS_ClrVal()
#define SPI_CS_DISABLE()  BLE_CS_SetVal()

#define SPI_IGNORED_BYTE          0xFEu /**< SPI default character. Character clocked out in case of an ignored transaction. */
#define SPI_OVERREAD_BYTE         0xFFu /**< SPI over-read character. Character clocked out after an over-read of the transmit buffer. */
#define SPI_DEFAULT_DELAY_US      50

static uint8_t rxDummy; /* dummy byte if we do not need the result. Needed to read from SPI register. */


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
#include "fsl_spi_hal.h"

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

//static uint8_t  _mode;
//static uint8_t  m_rx_buffer[SDEP_MAX_PACKETSIZE];
//static uint8_t  m_tx_buffer[SDEP_MAX_PACKETSIZE];
//static uint8_t  m_tx_count;

#define SPI_IGNORED_BYTE          0xFEu /**< SPI default character. Character clocked out in case of an ignored transaction. */
#define SPI_OVERREAD_BYTE         0xFFu /**< SPI over-read character. Character clocked out after an over-read of the transmit buffer. */
#define SPI_DEFAULT_DELAY_MS      1 // Default delay 1 millisecond

// static uint8_t rxDummy; /* dummy byte if we do not need the result. Needed to read from SPI register. */

/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
    kAdafruitBLESPIFriendPinMISO		= GPIO_MAKE_PIN(HW_GPIOA, 6), // PTA6
    kAdafruitBLESPIFriendPinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 7), // PTA7
    kAdafruitBLESPIFriendPinSCK		    = GPIO_MAKE_PIN(HW_GPIOB, 0), // PTB0
    kAdafruitBLESPIFriendPinCSn	        = GPIO_MAKE_PIN(HW_GPIOA, 5), // PTA5
    kAdafruitBLESPIFriendPinIRQ		    = GPIO_MAKE_PIN(HW_GPIOB, 6), // PTB6
};

//static int writeCommand(uint8_t commandByte, uint8_t commandByteSize)
//{
//    spi_status_t status;
//
//    /*
//     *	Drive /CS low.
//     *
//     *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
//     */
//    GPIO_DRV_SetPinOutput(kAdafruitBLESPIFriendPinCSn);
//    OSA_TimeDelay(10);
//    GPIO_DRV_ClearPinOutput(kAdafruitBLESPIFriendPinCSn);
//
//    status = SPI_DRV_MasterTransferBlocking(
//            0	/* master instance */,
//            NULL		/* spi_master_user_config_t */,
//            (const uint8_t * restrict)&commandByte,
//            (uint8_t * restrict)&m_rx_buffer[0],
//            commandByteSize		/* transfer size */,
//            1000		/* timeout in microseconds (unlike I2C which is ms) */
//            );
//
//    /*
//     *	Drive /CS high
//     */
//    GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
//
//    return status;
//}
//
//bool BLE_sendPacket(uint16_t command, const uint8_t *buf, uint8_t count, uint8_t more_data) {
//    sdepMsgCommand_t msgCmd;
//    bool result;
//
//    /* flush old response before sending the new command */
//    if (more_data == 0) {
//        m_rx_buffer[SDEP_MAX_PACKETSIZE] = 0u;
//    }
//    msgCmd.header.msg_type    = SDEP_MSGTYPE_COMMAND;
//    msgCmd.header.cmd_id_high = command>>8;
//    msgCmd.header.cmd_id_low  = command&0xff;
//    msgCmd.header.length      = count;
//    msgCmd.header.more_data   = (count == SDEP_MAX_PACKETSIZE) ? more_data : 0;
//
//    /* Copy payload */
//    if (buf != NULL && count > 0) {
//        memcpy(msgCmd.payload, buf, count);
//    }
//    GPIO_DRV_ClearPinOutput(kAdafruitBLESPIFriendPinCSn); // enable CS
//
//    /* Bluefruit may not be ready */
//    while ((spixfer(msgCmd.header.msg_type)==SPI_IGNORED_BYTE) && !TMOUT1_CounterExpired(timeoutHndl)) {
//        /* Disable & Re-enable CS with a bit of delay for Bluefruit to ready itself */
//        GPIO_DRV_SetPinOutput(kAdafruitBLESPIFriendPinCSn); // disable CS
//        OSA_TimeDelay(SPI_DEFAULT_DELAY_MS);
//        GPIO_DRV_ClearPinOutput(kAdafruitBLESPIFriendPinCSn); // enable CS
//    }
//
//    result = !TMOUT1_CounterExpired(timeoutHndl);
//    if (result) {
//        /* transfer the rest of the data */
//        spixferblock((void*)(((uint8_t*)&msgCmd)+1), sizeof(sdepMsgHeader_t)+count-1);
//    }
//    GPIO_DRV_SetPinOutput(kAdafruitBLESPIFriendPinCSn); // disable CS
//    return result;
//}

int devAdafruitBLESPIFriendInit(void)
{
    /*
     *	Override Warp firmware's use of these pins.
     *
     *
     */
    PORT_HAL_SetMuxMode(PORTA_BASE, 6u, kPortMuxAlt3);
    PORT_HAL_SetMuxMode(PORTA_BASE, 7u, kPortMuxAlt3);
    PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAlt3);

    warpEnableSPIpins();

    /*
     *	Override Warp firmware's use of these pins.
     *
     *	Reconfigure to use as GPIO.
     */
    PORT_HAL_SetMuxMode(PORTA_BASE, 5u, kPortMuxAsGpio);
    PORT_HAL_SetMuxMode(PORTB_BASE, 6u, kPortMuxAsGpio);
    return 0;
}

void printBLEReceivedMessage(void)
{
    // TODO: Maybe need to utilise IRQ pin
    spi_status_t status;
    uint8_t rx_buffer[16];
    uint8_t commandByte[16] = 0x10020A00;
    uint8_t commandByteSize = 16;

    commandByte[0] = 0x10u;
    commandByte[1] = 0x02u;
    commandByte[2] = 0x0Au;
    commandByte[3] = 0x00u;

    /*
     *	Drive /CS low.
     *
     *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
     */
    GPIO_DRV_SetPinOutput(kAdafruitBLESPIFriendPinCSn);
    OSA_TimeDelay(1);
    GPIO_DRV_ClearPinOutput(kAdafruitBLESPIFriendPinCSn);

    warpEnableSPIpins();

    warpPrint("command byte in hex: %x\n", commandByte);
    warpPrint("command byte in unsigned: %u\n", commandByte);
    warpPrint("command byte in decimal: %d\n", commandByte);
    status = SPI_DRV_MasterTransferBlocking(
            0	/* master instance */,
            NULL		/* spi_master_user_config_t */,
            (const uint8_t * restrict)&commandByte,
            (uint8_t * restrict)&rx_buffer,
            commandByteSize		/* transfer size */,
            1000		/* timeout in microseconds (unlike I2C which is ms) */
    );

    if (status == kStatus_SPI_Success)
    {
        warpPrint("SPI DRV Master Transfer Blocking is successful!\n");
    }
    else if (status == kStatus_SPI_Busy) {
        warpPrint("SPI is still busy!\n");
    }
    else if (status == kStatus_SPI_Timeout) {
        warpPrint("SPI DRV Master Transfer Blocking is timeout!\n");
    }
    else {
        warpPrint("SPI DRV Master Transfer Blocking is failed with status %d\n", status);
    }

    warpDisableSPIpins();
    /*
     *	Drive /CS high
     */
    GPIO_DRV_SetPinOutput(kAdafruitBLESPIFriendPinCSn);

    warpPrint("The result in string is %s\n", rx_buffer);
    warpPrint("The result in hex is is %x\n", rx_buffer);
}


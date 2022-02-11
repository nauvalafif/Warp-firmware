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
#include "fsl_gpio_driver.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"

#include "sdep.h"
#include "devAdafruitBLEUARTFriend..h"

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

// convert uint8_t to string, credit to https://stackoverflow.com/questions/27448168/conversion-of-uint8-t-to-a-string-c
char *convert(uint8_t *a)
{
    char* buffer2;
    int i;

    buffer2 = malloc(9);
    if (!buffer2)
        return NULL;

    buffer2[8] = 0;
    for (i = 0; i <= 7; i++)
        buffer2[7 - i] = (((*a) >> i) & (0x01)) + '0';

    return buffer2;
}

int devAdafruitBLESPIFriendInit(void)
{
    /*
     *	Override Warp firmware's use of these pins.
     *
     *
     */
    PORT_HAL_SetMuxMode(PORTA_BASE, 6u, kPortMuxAlt3); // MISO
    PORT_HAL_SetMuxMode(PORTA_BASE, 7u, kPortMuxAlt3); // MOSI
    PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAlt3); // SCK

    warpEnableSPIpins();

    /*
     *	Override Warp firmware's use of these pins.
     *
     *	Reconfigure to use as GPIO.
     */
    PORT_HAL_SetMuxMode(PORTA_BASE, 5u, kPortMuxAsGpio); // nCS
    PORT_HAL_SetMuxMode(PORTB_BASE, 6u, kPortMuxAsGpio); // IRQ

    PORT_HAL_SetPinIntMode(PORTB_BASE, 6u, kPortIntLogicOne);

    return 0;
}

void printBLEReceivedMessage(void)
{
    spi_status_t status;
    size_t commandByteSize = 20;
    uint8_t rx_buffer[commandByteSize];
    uint8_t commandByte[commandByteSize];
    char *temp;
    int i, j;

// Command for SDEP_CMDTYPE_BLE_UARTRX
    commandByte[0] = 0x10;
    commandByte[1] = 0x02;
    commandByte[2] = 0x0A;
    commandByte[3] = 0x00;

    // Command for SDEP_CMDTYPE_AT_WRAPPER a-t-I
//    commandByte[0] = 0x10;
//    commandByte[1] = 0x00;
//    commandByte[2] = 0x0A;
//    commandByte[3] = 0x03;
//    commandByte[4] = 0x61; // 'a'
//    commandByte[5] = 0x74; // 't'
//    commandByte[6] = 0x69; // 'i'

    if (PORT_HAL_IsPinIntPending(PORTB_BASE, 6u)) {
        warpPrint("Interrupt is detected!\n");
    } else {
        warpPrint("Interrupt is not detected!\n");
    }

    /*
     *	Drive /CS low.
     *
     *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
     */
    GPIO_DRV_SetPinOutput(kAdafruitBLESPIFriendPinCSn);
    OSA_TimeDelay(1);
    GPIO_DRV_ClearPinOutput(kAdafruitBLESPIFriendPinCSn);

    warpEnableSPIpins();
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

    GPIO_DRV_SetPinOutput(kAdafruitBLESPIFriendPinCSn);
    warpDisableSPIpins();
    /*
     *	Drive /CS high
     */

    warpPrint("The result in hex is: ");
    for (i = 0; i<commandByteSize; i++) {
        warpPrint("%02x", rx_buffer[i]);
    }
    warpPrint("\n");

    if (PORT_HAL_IsPinIntPending(PORTB_BASE, 6u)) {
        warpPrint("Interrupt is detected!\n");
    } else {
        warpPrint("Interrupt is not detected!\n");
    }
}


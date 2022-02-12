//
// Created by Muhammad Rizqi Nauval Afif on 11/02/2022.
//

/*
 *	config.h needs to come first
 */
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "devAdafruitBLEUARTFriend.h"

void enableUARTPins(void)
{
    /*
     *	Enable UART CLOCK. See KSDK13APIRM.pdf page 289
     */
    CLOCK_SYS_EnableUartClock(0);

    /*
     *	Set UART pin association. See, e.g., page 99 in
     *
     *		https://www.nxp.com/docs/en/reference-manual/KL03P24M48SF0RM.pdf
     *
     *	Setup:
     *		PTB3/kWarpPinI2C0_SCL_UART_TX for UART TX
     *		PTB4/kWarpPinI2C0_SCL_UART_RX for UART RX
     *		PTA7/kWarpPinSPI_MOSI_UART_CTS for UART CTS
     */

    PORT_HAL_SetMuxMode(PORTB_BASE, 3, kPortMuxAlt3);
    PORT_HAL_SetMuxMode(PORTB_BASE, 4, kPortMuxAlt3);
    PORT_HAL_SetMuxMode(PORTA_BASE, 7, kPortMuxAsGpio);
    GPIO_DRV_ClearPinOutput(kWarpPinSPI_MOSI_UART_CTS);

    /*
     *	Initialize UART0. See KSDK13APIRM.pdf page 2674 and 2701
     */

    uartConfig.baudRate = 9600;
    uartConfig.bitCountPerChar = kUart8BitsPerChar;
    uartConfig.parityMode = kUartParityDisabled;
    uartConfig.stopBitCount = kUartOneStopBit;

    UART_DRV_Init(0, &uartState, &uartConfig);
}

void initBLE(void)
{
    deviceBLEState.uartRXBuffer[0] = kWarpMiscMarkerForAbsentByte;
    /*
     *	Initialize UART and setup callback function.
     */
    uartState.txBuff = (uint8_t *)deviceBLEState.uartTXBuffer;
    uartState.rxBuff = (uint8_t *)deviceBLEState.uartRXBuffer;

    UART_DRV_InstallRxCallback(
            0,						/*	uint32_t instance		*/
            &uartRxCallback,		/*	uart_rx_callback_t function	*/
            (uint8_t *)deviceBLEState.uartRXBuffer,	    /*	uint8_t ∗ rxBuff		*/
            (void *)0, 				/*	void ∗callbackParam		*/
            1						/*	bool alwaysEnableRxIrq		*/
            );
}

void uartRxCallback(void) {}


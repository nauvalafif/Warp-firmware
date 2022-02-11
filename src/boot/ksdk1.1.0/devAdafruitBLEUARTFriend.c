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

#include "fsl_uart_driver.h"
#include "fsl_port_hal.h"
#include "fsl_gpio_driver.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"

#include "sdep.h"
#include "devAdafruitBLEUARTFriend.h"

void
enableUARTPins(void)
{
    /*
     *	Enable UART CLOCK
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

//TODO: we don't use hw flow control so don't need RTS/CTS
 *		PTA6/kWarpPinSPI_MISO_UART_RTS for UART RTS
 *		PTA7/kWarpPinSPI_MOSI_UART_CTS for UART CTS
     */
    PORT_HAL_SetMuxMode(PORTB_BASE, 3, kPortMuxAlt3);
    PORT_HAL_SetMuxMode(PORTB_BASE, 4, kPortMuxAlt3);

//TODO: we don't use hw flow control so don't need RTS/CTS
//	PORT_HAL_SetMuxMode(PORTA_BASE, 6, kPortMuxAsGpio);
//	PORT_HAL_SetMuxMode(PORTA_BASE, 7, kPortMuxAsGpio);
//	GPIO_DRV_SetPinOutput(kWarpPinSPI_MISO_UART_RTS);
//	GPIO_DRV_SetPinOutput(kWarpPinSPI_MOSI_UART_CTS);

    /*
     *	Initialize UART0. See KSDK13APIRM.pdf section 40.4.3, page 1353
     */
    uartUserConfig.baudRate = gWarpUartBaudRateBps;
    uartUserConfig.parityMode = kUartParityDisabled;
    uartUserConfig.stopBitCount = kUartOneStopBit;
    uartUserConfig.bitCountPerChar = kUart8BitsPerChar;
    uartUserConfig.clockSource = kClockUartSrcMcgIrClk;

    UART_DRV_Init(0,(uart_state_t *)&uartState,(uart_user_config_t *)&uartUserConfig);
}


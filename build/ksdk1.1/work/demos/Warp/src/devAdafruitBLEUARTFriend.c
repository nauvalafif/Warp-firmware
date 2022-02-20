//
// Created by Muhammad Rizqi Nauval Afif on 11/02/2022.
//

/*
 *	config.h needs to come first
 */
#include "config.h"

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fsl_lpuart_driver.h"
#include "fsl_port_hal.h"
#include "fsl_gpio_driver.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devAdafruitBLEUARTFriend.h"

lpuart_user_config_t uartConfig;
lpuart_state_t uartState;
extern volatile WarpUARTDeviceState	deviceBLEState;
int i;

void uartRxCallback() {}

void enableUARTPins(void)
{
    /*
     *	Enable UART CLOCK. See KSDK13APIRM.pdf page 289
     */
    CLOCK_SYS_EnableLpuartClock(0);

    /*
     *	Set UART pin association. See, e.g., page 99 in
     *
     *		https://www.nxp.com/docs/en/reference-manual/KL03P24M48SF0RM.pdf
     *
     *	Setup:
     *		PTB3/kWarpPinI2C0_SCL_UART_TX for UART TX
     *		PTB4/kWarpPinI2C0_SCL_UART_RX for UART RX
     *		CTS is not set but just grounded
     */

    PORT_HAL_SetMuxMode(PORTB_BASE, 3, kPortMuxAlt3); // UART TX connected to BLE RX
    PORT_HAL_SetMuxMode(PORTB_BASE, 4, kPortMuxAlt3); // UART RX connected to BLE TX

    /*
     *	Initialize UART0.
     */

    uartConfig.baudRate = 9600;
    uartConfig.parityMode = kLpuartParityDisabled;
    uartConfig.stopBitCount = kLpuartOneStopBit;
    uartConfig.bitCountPerChar = kLpuart8BitsPerChar;
    uartConfig.clockSource = kClockLpuartSrcMcgIrClk;

    LPUART_DRV_Init(0, &uartState, &uartConfig);
}

void initBLE(void)
{
    lpUARTStatus = kWarpMiscMarkerForAbsentByte;
    deviceBLEState.uartRXBuffer[0] = kWarpMiscMarkerForAbsentByte;
    /*
     *	Initialize UART and setup callback function.
     */
    uartState.txBuff = (uint8_t *)deviceBLEState.uartTXBuffer;
    uartState.rxBuff = (uint8_t *)deviceBLEState.uartRXBuffer;

    lpUARTStatus = LPUART_DRV_ReceiveDataBlocking(0,
                                   (uint8_t *)deviceBLEState.uartRXBuffer,
                                   kWarpSizesUartBufferBytes,
                                   kWarpDefaultUartTimeoutMilliseconds);
}

void disableUARTpins(void)
{
    /*
     *	LPUART deinit
     */
    LPUART_DRV_Deinit(0);

    /*
     *	Set UART pin association. See, e.g., page 99 in
     *
     *		https://www.nxp.com/docs/en/reference-manual/KL03P24M48SF0RM.pdf
     *
     *	Setup:
     *		PTB3/kWarpPinI2C0_SCL_UART_TX for UART TX
     *		PTB4/kWarpPinI2C0_SCL_UART_RX for UART RX
     */

    PORT_HAL_SetMuxMode(PORTB_BASE, 3, kPortPinDisabled);
    PORT_HAL_SetMuxMode(PORTB_BASE, 4, kPortPinDisabled);

    /*
     *	Disable LPUART CLOCK
    */
    CLOCK_SYS_DisableLpuartClock(0);

    for (i = 0; i<kWarpSizesUartBufferBytes; i++) {
        deviceBLEState.uartRXBuffer[i] = kWarpMiscMarkerForAbsentByte;
    }

    OSA_TimeDelay(10);
}


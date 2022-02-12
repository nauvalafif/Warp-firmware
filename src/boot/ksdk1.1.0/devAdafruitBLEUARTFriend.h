//
// Created by Muhammad Rizqi Nauval Afif on 11/02/2022.
//

#include "fsl_uart_driver.h"
#include "fsl_port_hal.h"
#include "fsl_gpio_driver.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"

#include "sdep.h"

extern volatile uart_user_config_t uartConfig;
extern volatile uart_state_t uartState;
extern volatile WarpUARTDeviceState	deviceBLEState;

void enableUARTPins(void);
void initBLE(void);
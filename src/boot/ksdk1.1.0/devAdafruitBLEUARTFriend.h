//
// Created by Muhammad Rizqi Nauval Afif on 11/02/2022.
//

extern volatile uart_user_config_t uartConfig;
extern volatile uart_state_t uartState;
extern volatile WarpUARTDeviceState	deviceBLEState;

void enableUARTPins(void);
void initBLE(void);
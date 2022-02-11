//
// Created by Muhammad Rizqi Nauval Afif on 05/01/2022.
//

// Credit to Erich Styger https://github.com/ErichStyger/mcuoneclipse/tree/master/Examples/KDS/tinyK20/tinyK20_Adafruit_BLE

//#include "sdep.h"
#include "fsl_spi_hal.h"

//uint8_t BLE_SendATCommandExpectedResponse(const uint8_t *cmd, uint8_t *rxBuf, size_t rxBufSize, const uint8_t *expectedTailStr);
//bool BLE_sendPacket(uint16_t command, const uint8_t *buf, uint8_t count, uint8_t more_data);
//bool BLE_getResponse(void);
//uint8_t BLE_Echo(bool on);
//bool BLE_IsConnected(void);
//uint8_t BLE_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);
//void BLE_Init(void);

int devAdafruitBLESPIFriendInit(void);
void printBLEReceivedMessage(void);
char *convert(uint8_t *a);
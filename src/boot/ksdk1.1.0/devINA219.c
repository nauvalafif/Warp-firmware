//
// Created by Muhammad Rizqi Nauval Afif on 16/11/2021.
//
#include <stdlib.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

#include "fsl_i2c_master_driver.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"
#include "devINA219.h"

extern volatile WarpI2CDeviceState	deviceINA219State;
extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;

void
initINA219(const uint8_t i2cAddress, uint16_t operatingVoltageMillivolts)
{
    deviceINA219State.i2cAddress			= i2cAddress; // INA219 default I2C address
    deviceINA219State.operatingVoltageMillivolts	= operatingVoltageMillivolts;

    return;
}

//union unionType
//{
//    unsigned int intVar;
//    unsigned char bytes[2];
//} temp;

WarpStatus
writeSensorRegisterINA219(uint8_t deviceRegister, uint16_t payload)
{
    uint8_t		payloadByte[2], commandByte[1];
    i2c_status_t	status;

    switch (deviceRegister)
    {
        case kWarpSensorConfigurationRegisterINA219:
        case kWarpSensorCalibrationRegisterINA219:
        {
            /* OK */
            break;
        }

        default:
        {
            return kWarpStatusBadDeviceCommand;
        }
    }

    i2c_device_t slave =
            {
                    .address = deviceINA219State.i2cAddress,
                    .baudRate_kbps = gWarpI2cBaudRateKbps
            };

    warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);
    commandByte[0] = deviceRegister;
    payloadByte[0] = (payload>>8) & 0xFF; /* MSB first */
    payloadByte[1] = payload & 0xFF; /* LSB */
    warpEnableI2Cpins();

    status = I2C_DRV_MasterSendDataBlocking(
            0 /* I2C instance */,
            &slave,
            commandByte,
            1,
            payloadByte,
            2,
            gWarpI2cTimeoutMilliseconds);
    if (status != kStatus_I2C_Success)
    {
        return kWarpStatusDeviceCommunicationFailed;
    }

    return kWarpStatusOK;
}

//WarpStatus
//calibrateSensorINA219(uint16_t payloadCalibrate)
//{
//    WarpStatus	i2cWriteStatus;
//
//
//    warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);
//
//    i2cWriteStatus = writeSensorRegisterINA219(kWarpSensorCalibrationRegisterINA219 /* register address for calibration */,
//                                                  payloadCalibrate /* payload */
//    );
//
//    return (i2cWriteStatus);
//}

WarpStatus
readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes)
{
    uint8_t		cmdBuf[1] = {0xFF};
    i2c_status_t	status;


    USED(numberOfBytes);
    switch (deviceRegister)
    {
        case kWarpSensorOutputRegisterINA219_Current:

        {
            /* OK */
            break;
        }

        default:
        {
            return kWarpStatusBadDeviceCommand;
        }
    }

    i2c_device_t slave =
            {
                    .address = deviceINA219State.i2cAddress,
                    .baudRate_kbps = gWarpI2cBaudRateKbps
            };

    warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);
    cmdBuf[0] = deviceRegister;
    warpEnableI2Cpins();

    status = I2C_DRV_MasterReceiveDataBlocking(
            0 /* I2C peripheral instance */,
            &slave,
            cmdBuf,
            1,
            (uint8_t *)deviceINA219State.i2cBuffer,
            numberOfBytes,
            gWarpI2cTimeoutMilliseconds);

    if (status != kStatus_I2C_Success)
    {
        return kWarpStatusDeviceCommunicationFailed;
    }

    return kWarpStatusOK;
}

void
printSensorDataINA219(bool hexModeFlag)
{
    uint16_t	readSensorRegisterValueLSB;
    uint16_t	readSensorRegisterValueMSB;
    int16_t		readSensorRegisterValueCombined;
    WarpStatus	i2cReadStatus;


    warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);

    /*
     *	From the INA219 datasheet:
     *
     *		"A random read access to the LSB registers is not possible.
     *		Reading the MSB register and then the LSB register in sequence
     *		ensures that both bytes (LSB and MSB) belong to the same data
     *		sample, even if a new data sample arrives between reading the
     *		MSB and the LSB byte."
     *
     *	We therefore do 2-byte read transactions, for each of the registers.
     *	We could also improve things by doing a 6-byte read transaction.
     */
    i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219_Current, 2 /* numberOfBytes */);
    readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
    readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
    readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 6) | (readSensorRegisterValueLSB >> 2);

    /*
     *	Sign extend the 14-bit value based on knowledge that upper 2 bit are 0:
     */
    readSensorRegisterValueCombined = (readSensorRegisterValueCombined ^ (1 << 13)) - (1 << 13);

    if (i2cReadStatus != kWarpStatusOK)
    {
        warpPrint(" ----,");
    }
    else
    {
        if (hexModeFlag)
        {
            warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
        }
        else
        {
            warpPrint(" %d,", readSensorRegisterValueCombined);
        }
    }
}
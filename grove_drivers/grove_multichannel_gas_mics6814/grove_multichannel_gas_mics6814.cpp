/*
 * grove_multichannel_gas_mics6814.cpp
 *
 * Copyright (c) 2012 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Jacky Zhang
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "suli2.h"
#include <math.h>
#include "grove_multichannel_gas_mics6814.h"


GroveMultiChannelGas::GroveMultiChannelGas(int pinsda, int pinscl)
{
    this->i2c = (I2C_T *)malloc(sizeof(I2C_T));
    suli_i2c_init(this->i2c, pinsda, pinscl);
    
    //get the default address of device. user should use his own address he set to the device previously.
    i2cAddress = MULTICHANNEL_GAS_ADDRESS;
    
    is_connected = 0;
    if (readR0() >= 0) is_connected = 1;

    powerOn();
}

/*********************************************************************************************************
** Function name:           sendCmd
** Descriptions:            send one byte to I2C Wire
*********************************************************************************************************/
void GroveMultiChannelGas::sendCmd(uint8_t dta)
{
    suli_i2c_write(this->i2c, i2cAddress, &dta, 1);
}

/*********************************************************************************************************
** Function name:           readData
** Descriptions:            read 4 bytes from I2C slave
*********************************************************************************************************/
int16_t GroveMultiChannelGas::readData(uint8_t cmd)
{
    uint16_t timeout = 0;
    uint8_t buffer[4];
    uint8_t checksum = 0;
    int16_t rtnData = 0;

    //send command
    sendCmd(cmd);
    
    //wait for a while, because the device is a MCU, it need some time to handle the request.
    suli_delay_ms(2);
    
    //read sensor data
    int length = suli_i2c_read(this->i2c, i2cAddress, buffer, 4);
    if(length != 4)
    {
         return -3;//rtnData length wrong
    }
    //checksum
    checksum = (uint8_t)(buffer[0] + buffer[1] + buffer[2]);
    if(checksum != buffer[3])
        return -4;//checksum wrong
    
    rtnData = ((buffer[1] << 8) + buffer[2]);
    
    return rtnData;//successful
}

/*********************************************************************************************************
** Function name:           readR0
** Descriptions:            read R0 stored in slave MCU
*********************************************************************************************************/
int16_t GroveMultiChannelGas::readR0()
{
    int16_t rtnData = 0;

    rtnData = readData(READ_R0_CH1);
    if(rtnData >= 0)
        res0[0] = rtnData;
    else
        return rtnData;//unsuccessful

    rtnData = readData(READ_R0_CH2);
    if(rtnData >= 0)
        res0[1] = rtnData;
    else
        return rtnData;//unsuccessful

    rtnData = readData(READ_R0_CH3);
    if(rtnData >= 0)
        res0[2] = rtnData;
    else
        return rtnData;//unsuccessful
    
    for(int i=0; i<3; i++)
    {
        if (res0[i] == 0) res0[i] = 0.000001f;
    }
    
    return 0;//successful
}

/*********************************************************************************************************
** Function name:           readR 
** Descriptions:            read resistance value of each channel from slave MCU
*********************************************************************************************************/
int16_t GroveMultiChannelGas::readR()
{
    int16_t rtnData = 0;

    rtnData = readData(READ_RS_CH1);
    if(rtnData >= 0)
        res[0] = rtnData;
    else
        return rtnData;//unsuccessful

    rtnData = readData(READ_RS_CH2);
    if(rtnData >= 0)
        res[1] = rtnData;
    else
        return rtnData;//unsuccessful

    rtnData = readData(READ_RS_CH3);
    if(rtnData >= 0)
        res[2] = rtnData;
    else
        return rtnData;//unsuccessful
    
    return 0;//successful
}

/*********************************************************************************************************
** Function name:           calcGas
** Descriptions:            calculate gas concentration of each channel from slave MCU
** Parameters:
                            gas - gas type
** Returns:
                            float value - concentration of the gas
*********************************************************************************************************/
float GroveMultiChannelGas::calcGas(int gas)
{
    if(!is_connected)
    {
        if(readR0() >= 0) is_connected = 1;
        else
        {
            error_desc = "The sensor is not connected.";
            return -1.0f;
        }
    }

    if (readR() < 0)
    {
        error_desc = "Error when communicating with the sensor.";
        return -2.0f;
    }

    float ratio0 = (float)res[0] / res0[0];
    float ratio1 = (float)res[1] / res0[1];
    float ratio2 = (float)res[2] / res0[2];

    float c = 0;

    switch(gas)
    {
        case CO:
        {
            if(ratio1 < 0.01) ratio1 = 0.01;
            if(ratio1 > 3) ratio1 = 3;
            //c = pow(10, 0.6) / pow(ratio1, 1.2);
            c = pow(ratio1, -1.179)*4.385;  //mod by jack
            break;
        }
        case NO2:
        {
            if(ratio2 < 0.07) ratio2 = 0.07;
            if(ratio2 > 40) ratio2 = 40;
            //c = ratio2 / pow(10, 0.8);
            c = pow(ratio2, 1.007)/6.855;  //mod by jack
            break;
        }
        case NH3:
        {
            if(ratio0 < 0.04) ratio0 = 0.04;
            if(ratio0 > 0.8) ratio0 = 0.8;
            //c = 1 / (ratio0 * ratio0 * pow(10, 0.4));
            c = pow(ratio0, -1.67)/1.47;  //modi by jack
            break;
        }
        case C3H8:  //add by jack
        {
            if(ratio0 < 0.23) ratio0 = 0.23;
            if(ratio0 > 0.8) ratio0 = 0.8;
            c = pow(ratio0, -2.518)*570.164;
            break;
        }
        case C4H10:  //add by jack
        {
            if(ratio0 < 0.15) ratio0 = 0.15;
            if(ratio0 > 0.65) ratio0 = 0.65;
            c = pow(ratio0, -2.138)*398.107;
            break;
        }
        case CH4:  //add by jack
        {
            if(ratio1 < 0.5) ratio1 = 0.5;
            if(ratio1 > 0.7) ratio1 = 0.7;
            c = pow(ratio1, -4.363)*630.957;
            break;
        }
        case H2:  //add by jack
        {
            if(ratio1 < 0.04) ratio1 = 0.04;
            if(ratio1 > 0.8) ratio1 = 0.8;
            c = pow(ratio1, -1.8)*0.73;
            break;
        }
        case C2H5OH:  //add by jack
        {
            if(ratio1 < 0.04) ratio1 = 0.04;
            if(ratio1 > 1.1) ratio1 = 1.1;
            c = pow(ratio1, -1.552)*1.622;
            break;
        }
        default:
            break;
    }

    return isnan(c)?-3:c;
}

/*********************************************************************************************************
** Function name:           changeI2cAddr
** Descriptions:            change I2C address of the slave MCU, and this address will be stored in EEPROM of slave MCU
*********************************************************************************************************/
void GroveMultiChannelGas::changeI2cAddr(uint8_t newAddr)
{
    //Wire.beginTransmission(i2cAddress); // transmit to device
    //Wire.write(CHANGE_I2C_ADDR);              // sends one byte
    //Wire.write(newAddr);              // sends one byte
    //Wire.endTransmission();    // stop transmitting
    
    uint8_t data[2];
    data[0] = CHANGE_I2C_ADDR;
    data[1] = newAddr;

    suli_i2c_write(this->i2c, i2cAddress, data, 2);
    i2cAddress = newAddr;
}

/*********************************************************************************************************
** Function name:           doCalibrate
** Descriptions:            tell slave to do a calibration, it will take about 8~10s
*********************************************************************************************************/
void GroveMultiChannelGas::doCalibrate(void)
{
    sendCmd(DO_CALIBARTE);

    suli_delay_ms(8000);
    
    if(readR0() >= 0) is_connected = 1;
    else is_connected = 0;
}

/*********************************************************************************************************
** Function name:           powerOn
** Descriptions:            power on sensor heater
*********************************************************************************************************/
void GroveMultiChannelGas::powerOn(void)
{
    sendCmd(POWER_ON);
}

/*********************************************************************************************************
** Function name:           powerOff
** Descriptions:            power off sensor heater
*********************************************************************************************************/
void GroveMultiChannelGas::powerOff(void)
{
    sendCmd(POWER_OFF);
}

/*********************************************************************************************************
** Function name:           read_xxx
** Descriptions:            measure the concentration of xxx, with unit ppm
*********************************************************************************************************/
bool GroveMultiChannelGas::read_CO(float *concentration)
{
    *concentration = calcGas(CO);
    return true;    
}
bool GroveMultiChannelGas::read_NO2(float *concentration)
{
    *concentration = calcGas(NO2);
    if (*concentration >= 0) return true;
    else return false;
}
bool GroveMultiChannelGas::read_NH3(float *concentration)
{
    *concentration = calcGas(NH3);
    if (*concentration >= 0) return true;
    else return false;
}
bool GroveMultiChannelGas::read_C3H8(float *concentration)
{
    *concentration = calcGas(C3H8);
    if (*concentration >= 0) return true;
    else return false;
}
bool GroveMultiChannelGas::read_C4H10(float *concentration)
{
    *concentration = calcGas(C4H10);
    if (*concentration >= 0) return true;
    else return false;
}
bool GroveMultiChannelGas::read_CH4(float *concentration)
{
    *concentration = calcGas(CH4);
    if (*concentration >= 0) return true;
    else return false;
}
bool GroveMultiChannelGas::read_H2(float *concentration)
{
    *concentration = calcGas(H2);
    if (*concentration >= 0) return true;
    else return false;
}
bool GroveMultiChannelGas::read_C2H5OH(float *concentration)
{
    *concentration = calcGas(C2H5OH);
    if (*concentration >= 0) return true;
    else return false;
}



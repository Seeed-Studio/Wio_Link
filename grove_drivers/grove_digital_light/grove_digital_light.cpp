/*
 * grove_digital_light.cpp
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
#include "grove_digital_light.h"



GroveDigitalLight::GroveDigitalLight(int pinsda, int pinscl)
{
    this->i2c = (I2C_T *)malloc(sizeof(I2C_T));

    suli_i2c_init(i2c, pinsda, pinscl);

    cmdbuf[0] = TSL2561_Control;
    cmdbuf[1] = 0x03;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 2);  // POWER UP
    cmdbuf[0] = TSL2561_Timing;
    cmdbuf[1] = 0x00;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 2);  //No High Gain (1x), integration time of 13ms
    cmdbuf[0] = TSL2561_Interrupt;
    cmdbuf[1] = 0x00;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 2);
    cmdbuf[0] = TSL2561_Control;
    cmdbuf[1] = 0x00;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 2);  // POWER Down

}

void GroveDigitalLight::_getLux(I2C_T *i2c)
{
    uint8_t CH0_LOW = 0, CH0_HIGH = 0, CH1_LOW = 0, CH1_HIGH = 0;

    cmdbuf[0] = TSL2561_Channal0L;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 1);
    suli_i2c_read(i2c, TSL2561_Address, &CH0_LOW, 1);
    cmdbuf[0] = TSL2561_Channal0H;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 1);
    suli_i2c_read(i2c, TSL2561_Address, &CH0_HIGH, 1);
    cmdbuf[0] = TSL2561_Channal1L;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 1);
    suli_i2c_read(i2c, TSL2561_Address, &CH1_LOW, 1);
    cmdbuf[0] = TSL2561_Channal1H;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 1);
    suli_i2c_read(i2c, TSL2561_Address, &CH1_HIGH, 1);

    ch0 = (CH0_HIGH<<8) | CH0_LOW;
    ch1 = (CH1_HIGH<<8) | CH1_LOW;
}

unsigned long GroveDigitalLight::_calculateLux(unsigned int iGain, unsigned int tInt, int iType)
{
    switch (tInt)
    {
        case 0:  // 13.7 msec
            chScale = CHSCALE_TINT0;
            break;
        case 1: // 101 msec
            chScale = CHSCALE_TINT1;
            break;
        default: // assume no scaling
            chScale = (1 << CH_SCALE);
            break;
    }
    if (!iGain)  chScale = chScale << 4; // scale 1X to 16X
    // scale the channel values
    channel0 = (ch0 * chScale) >> CH_SCALE;
    channel1 = (ch1 * chScale) >> CH_SCALE;

    ratio1 = 0;
    if (channel0!= 0) ratio1 = (channel1 << (RATIO_SCALE+1))/channel0;
    // round the ratio value
    unsigned long ratio = (ratio1 + 1) >> 1;

    switch (iType)
    {
        case 0: // T package
            if ((ratio >= 0) && (ratio <= K1T))
            {b=B1T; m=M1T;}
            else if (ratio <= K2T)
            {b=B2T; m=M2T;}
            else if (ratio <= K3T)
            {b=B3T; m=M3T;}
            else if (ratio <= K4T)
            {b=B4T; m=M4T;}
            else if (ratio <= K5T)
            {b=B5T; m=M5T;}
            else if (ratio <= K6T)
            {b=B6T; m=M6T;}
            else if (ratio <= K7T)
            {b=B7T; m=M7T;}
            else if (ratio > K8T)
            {b=B8T; m=M8T;}
            break;
        case 1:// CS package
            if ((ratio >= 0) && (ratio <= K1C))
            {b=B1C; m=M1C;}
            else if (ratio <= K2C)
            {b=B2C; m=M2C;}
            else if (ratio <= K3C)
            {b=B3C; m=M3C;}
            else if (ratio <= K4C)
            {b=B4C; m=M4C;}
            else if (ratio <= K5C)
            {b=B5C; m=M5C;}
            else if (ratio <= K6C)
            {b=B6C; m=M6C;}
            else if (ratio <= K7C)
            {b=B7C; m=M7C;}
    }
    temp=((channel0*b)-(channel1*m));
    if(temp<0) temp=0;
    temp+=(1<<(LUX_SCALE-1));
    // strip off fractional portion
    lux=temp>>LUX_SCALE;
    return (lux);
}

bool GroveDigitalLight::read_lux(uint32_t *lux)
{
    cmdbuf[0] = TSL2561_Control;
    cmdbuf[1] = 0x03;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 2);  // POWER UP
    suli_delay_ms(14);
    _getLux(i2c);

    cmdbuf[0] = TSL2561_Control;
    cmdbuf[1] = 0x00;
    suli_i2c_write(i2c, TSL2561_Address, cmdbuf, 2);  // POWER Down

    if (ch0 == 0 && ch1 == 0)
    {
        *lux = 0;
        return true;
    }

    if(ch1 !=0 && ch0/ch1 < 2 && ch0 > 4900)
    {
        error_desc = "The exposure value is out of range, the light is too strong.";
        return false;  //ch0 out of range, but ch1 not. the lux is not valid in this situation.
    }
    *lux = _calculateLux(0, 0, 0);  //T package, no gain, 13ms
    return true;
}


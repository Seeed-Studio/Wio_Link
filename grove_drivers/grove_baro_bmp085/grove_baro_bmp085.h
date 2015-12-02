/*
 * grove_baro_bmp085.h
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


#ifndef __GROVE_BARO_BMP085_H__
#define __GROVE_BARO_BMP085_H__

#include "suli2.h"

//GROVE_NAME        "Grove-Barometer(BMP085)"
//SKU               101020032
//IF_TYPE           I2C
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/thumb/e/e7/Grove-Barometer.jpg/621px-Grove-Barometer.jpg



#define BMP085_ADDRESS (0x77<<1)

class GroveBaroBMP085
{
public:
    GroveBaroBMP085(int pinsda, int pinscl);
    
    /**
     * Read a rough temperature value of the envirenment
     * 
     * @param temperature - unit: Celsius degree
     * 
     * @return bool 
     */
    bool read_temperature(float *temperature);
    
    /**
     * 
     * 
     * @param pressure - unit: Pa
     * 
     * @return bool 
     */
    bool read_pressure(int32_t *pressure);
    
    /**
     * 
     * 
     * @param altitude - the absolute altitude, unit: m
     * 
     * @return bool 
     */
    bool read_altitude(float *altitude);
private:
    I2C_T *i2c;
    uint8_t cmdbuf[2];
    uint8_t databuf[2];
    const uint8_t OSS = 0;  //0: lowpower 1: standard 2: high 3: ultrahigh accuration
    int16_t ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t b1, b2;
    int16_t mb, mc, md;
    int32_t PressureCompensate;

    uint8_t _read_char(unsigned char addr);
    uint16_t _read_int(unsigned char addr);
    int32_t _readut(I2C_T *i2c);
    int32_t _readup(I2C_T *i2c);

};

#endif

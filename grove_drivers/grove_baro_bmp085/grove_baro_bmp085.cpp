/*
 * grove_baro_bmp085.cpp
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
#include "grove_baro_bmp085.h"

GroveBaroBMP085::GroveBaroBMP085(int pinsda, int pinscl)
{
    this->i2c = (I2C_T *)malloc(sizeof(I2C_T));
    suli_i2c_init(i2c, pinsda, pinscl);

    ac1 = _read_int(0xAA);
    ac2 = _read_int(0xAC);
    ac3 = _read_int(0xAE);
    ac4 = _read_int(0xB0);
    ac5 = _read_int(0xB2);
    ac6 = _read_int(0xB4);
    b1 =  _read_int(0xB6);
    b2 =  _read_int(0xB8);
    mb =  _read_int(0xBA);
    mc =  _read_int(0xBC);
    md =  _read_int(0xBE);

}

// Read 1 byte from the BMP085
uint8_t GroveBaroBMP085::_read_char(unsigned char addr)
{
    //suli_i2c_write(i2c, BMP085_ADDRESS, &addr, 1);
    //suli_i2c_read(i2c, BMP085_ADDRESS, databuf, 1);
    suli_i2c_read_reg(i2c, BMP085_ADDRESS, addr, databuf, 1);
    return databuf[0];
}

// Read 2 bytes from the BMP085
uint16_t GroveBaroBMP085::_read_int(unsigned char addr)
{
    //suli_i2c_write(i2c, BMP085_ADDRESS, &addr, 1);
    //suli_i2c_read(i2c, BMP085_ADDRESS, databuf, 2);
    suli_i2c_read_reg(i2c, BMP085_ADDRESS, addr, databuf, 2);
    return (databuf[0] << 8) + databuf[1];
}

// Read the uncompensated temperature value
int32_t GroveBaroBMP085::_readut(I2C_T *i2c)
{
    uint16_t ut;

//    Wire.beginTransmission(BMP085_ADDRESS);
//    Wire.write(0xF4);
//    Wire.write(0x2E);
//    Wire.endTransmission();
    cmdbuf[0] = 0xF4;
    cmdbuf[1] = 0x2E;
    suli_i2c_write(i2c, BMP085_ADDRESS, cmdbuf, 2);  // POWER UP


    suli_delay_ms(5);
    //ut = bmp085ReadInt(0xF6);
    ut = _read_int(0xF6);

    return ut;
}

// Read the uncompensated pressure value
int32_t GroveBaroBMP085::_readup(I2C_T *i2c)
{
    uint8_t msb, lsb, xlsb;
    uint32_t up = 0;

//    Wire.beginTransmission(BMP085_ADDRESS);
//    Wire.write(0xF4);
//    Wire.write(0x34 + (OSS<<6));
//    Wire.endTransmission();
    cmdbuf[0] = 0xF4;
    cmdbuf[1] = 0x34 + (OSS<<6);
    suli_i2c_write(i2c, BMP085_ADDRESS, cmdbuf, 2);  // POWER UP


    suli_delay_ms(2 + (3 << OSS));

    // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
//    msb = bmp085Read(0xF6);
//    lsb = bmp085Read(0xF7);
//    xlsb = bmp085Read(0xF8);
    msb = _read_char( 0xF6);
    lsb = _read_char( 0xF7);
    xlsb = _read_char( 0xF8);

    up = (( msb << 16) | ( lsb << 8) |  xlsb) >> (8-OSS);

    return up;
}

bool GroveBaroBMP085::read_temperature(float *temperature)
{
    int32_t x1, x2;
    int32_t ut = _readut(i2c);

    x1 = ((ut - ac6) * ac5) >> 15;
    x2 = (mc << 11) / (x1 + md);
    PressureCompensate = x1 + x2;

    float temp = ((PressureCompensate + 8)>>4);
    *temperature = temp /10;

    return true;
}

bool GroveBaroBMP085::read_pressure(int32_t *pressure)
{
    int32_t x1, x2, x3, b3, b6, p;
    uint32_t b4, b7;
    int32_t up = _readup(i2c);

    b6 = PressureCompensate - 4000;
    x1 = (b2 * (b6 * b6)>>12)>>11;
    x2 = (ac2 * b6)>>11;
    x3 = x1 + x2;
    b3 = (((ac1*4 + x3)<<OSS) + 2)>>2;

    // Calculate B4
    x1 = (ac3 * b6)>>13;
    x2 = (b1 * ((b6 * b6)>>12))>>16;
    x3 = ((x1 + x2) + 2)>>2;
    b4 = (ac4 * (uint32_t)(x3 + 32768)) >> 15;

    b7 = ((uint32_t)(up - b3) * (50000 >> OSS));
    if (b7 < 0x80000000)
        p = (b7<<1)/b4;
    else
        p = (b7/b4)<<1;

    x1 = (p>>8) * (p>>8);
    x1 = (x1 * 3038)>>16;
    x2 = (-7357 * p)>>16;
    p += (x1 + x2 + 3791)>>4;

    *pressure = p;
    return true;
}

bool GroveBaroBMP085::read_altitude(float *altitude)
{
    float t;
    int32_t p;
    read_temperature(&t);
    read_pressure(&p);
    //Serial1.println(p);
    float A = (float)p/101325.0;
    float B = 1/5.25588;
    float C = pow(A,B);
    C = 1 - C;
    C = C /0.0000225577;
    *altitude = C;
    return true;
}


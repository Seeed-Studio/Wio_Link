/*
 * grove_gyro_itg3200.h
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



#ifndef __GROVE_GYROSCOPE_LIGHT_H__
#define __GROVE_GYROSCOPE_LIGHT_H__

//GROVE_NAME        "Grove-3-Axis Digital Gyro"
//SKU               101020050
//IF_TYPE           I2C
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/thumb/f/f6/Gbgr.jpg/500px-Gbgr.jpg


#include "suli2.h"

#define GYRO_ADDRESS (0x68<<1)

// ITG3200 Register Defines
#define ITG3200_WHO     0x00
#define ITG3200_SMPL    0x15
#define ITG3200_DLPF    0x16
#define ITG3200_INT_C   0x17
#define ITG3200_INT_S   0x1A
#define ITG3200_TMP_H   0x1B
#define ITG3200_TMP_L   0x1C
#define ITG3200_GX_H    0x1D
#define ITG3200_GX_L    0x1E
#define ITG3200_GY_H    0x1F
#define ITG3200_GY_L    0x20
#define ITG3200_GZ_H    0x21
#define ITG3200_GZ_L    0x22
#define ITG3200_PWR_M   0x3E


class GroveGyroITG3200
{
public:
    GroveGyroITG3200(int pinsda, int pinscl);
    
    /**
     * 
     * 
     * @param gx unit: deg/s
     * @param gy unit: deg/s
     * @param gz unit: deg/s
     * 
     * @return bool 
     */
    bool read_gyro(float *gx, float *gy, float *gz);  //unit: deg/s
    
    /**
     * 
     * 
     * @param temp unit: Celsius degree
     * 
     * @return bool 
     */
    bool read_temperature(float *temp);  //unit: C
    
    /**
     * Calibrate the static offsets, need to hold the sensor steadily.
     * 
     * @return bool 
     */
    bool write_zerocalibrate();

private:
    I2C_T *i2c;

    char _read_char(I2C_T *i2c, unsigned char addr);
    void _getxyz(I2C_T *i2c, int16_t *x, int16_t *y, int16_t *z);
    void _zerocalibrate(int sample_cnt);

    unsigned char cmdbuf[2];
    unsigned char databuf[2];
    int16_t x_offset;
    int16_t y_offset;
    int16_t z_offset;

};


#endif

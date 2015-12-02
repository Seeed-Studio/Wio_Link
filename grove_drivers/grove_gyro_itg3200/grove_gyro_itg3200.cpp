/*
 * grove_gyro_itg3200.cpp
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
#include "grove_gyro_itg3200.h"

GroveGyroITG3200::GroveGyroITG3200(int pinsda, int pinscl)
{
    this->i2c = (I2C_T *)malloc(sizeof(I2C_T));

    suli_i2c_init(i2c, pinsda, pinscl);

    cmdbuf[0] = ITG3200_PWR_M;
    cmdbuf[1] = 0x80;
    suli_i2c_write(i2c, GYRO_ADDRESS, cmdbuf, 2); //send a reset to the device
    cmdbuf[0] = ITG3200_SMPL;
    cmdbuf[1] = 0x00;
    suli_i2c_write(i2c, GYRO_ADDRESS, cmdbuf, 2); //sample rate divider
    cmdbuf[0] = ITG3200_DLPF;
    cmdbuf[1] = 0x18;
    suli_i2c_write(i2c, GYRO_ADDRESS, cmdbuf, 2); //+/-2000 degrees/s (default value)
    
    //do very rough and quick calibration
    _zerocalibrate(10);

}

// Read 1 byte from I2C
char GroveGyroITG3200::_read_char(I2C_T *i2c, unsigned char addr)
{
    suli_i2c_write(i2c, GYRO_ADDRESS, &addr, 1);
    suli_i2c_read(i2c, GYRO_ADDRESS, databuf, 1);
    return databuf[0];
}

/*Function: Get the temperature from ITG3200 that with a on-chip*/
/*           temperature sensor.                                */
bool GroveGyroITG3200::read_temperature(float *temp111)
{
    int16_t temp;
    float temperature;
    //temp = read(ITG3200_TMP_H, ITG3200_TMP_L);
    temp = (_read_char(i2c, ITG3200_TMP_H) << 8) + _read_char(i2c, ITG3200_TMP_L);
    temperature = 35 + ((float)(temp + 13200)) / 280;
    *temp111 = temperature;
    return true;
}

/*Function: Get the contents of the registers in the ITG3200*/
/*          so as to calculate the angular velocity.        */
void GroveGyroITG3200::_getxyz(I2C_T *i2c, int16_t *x, int16_t *y, int16_t *z)
{
    *x = (_read_char(i2c, ITG3200_GX_H) << 8) + _read_char(i2c, ITG3200_GX_L) - x_offset;
    *y = (_read_char(i2c, ITG3200_GY_H) << 8) + _read_char(i2c, ITG3200_GY_L) - y_offset;
    *z = (_read_char(i2c, ITG3200_GZ_H) << 8) + _read_char(i2c, ITG3200_GZ_L) - z_offset;
}

/*Function: Get the angular velocity and its unit is degree per second.*/
bool GroveGyroITG3200::read_gyro(float *ax, float *ay, float *az)
{
    int16_t x, y, z;
    _getxyz(i2c, &x, &y, &z);
    *ax = x / 14.375;
    *ay = y / 14.375;
    *az = z / 14.375;

    return true;
}

void GroveGyroITG3200::_zerocalibrate(int sample_cnt)
{
    int16_t x_offset_temp;
    int16_t y_offset_temp;
    int16_t z_offset_temp;
    int16_t x, y, z;
    x_offset = 0;
    y_offset = 0;
    z_offset = 0;

    for (int i = 0; i < sample_cnt; i++)
    {
        suli_delay_ms(1);
        _getxyz(i2c, &x, &y, &z);
        x_offset_temp += x;
        y_offset_temp += y;
        z_offset_temp += z;
    }

    x_offset = x_offset_temp / sample_cnt;
    y_offset = y_offset_temp / sample_cnt;
    z_offset = z_offset_temp / sample_cnt;
    //if (x_offset_temp > 0) x_offset = -x_offset;
    //if (y_offset_temp > 0) y_offset = -y_offset;
    //if (z_offset_temp > 0) z_offset = -z_offset;
}

bool GroveGyroITG3200::write_zerocalibrate()
{
    _zerocalibrate(100);

    return true;
}

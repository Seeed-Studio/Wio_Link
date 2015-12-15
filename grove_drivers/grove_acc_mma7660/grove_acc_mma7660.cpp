/*
 * grove_acc_mma7660.cpp
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
#include "grove_acc_mma7660.h"

GroveAccMMA7660::GroveAccMMA7660(int pinsda, int pinscl)
{
    this->i2c = (I2C_T *)malloc(sizeof(I2C_T));
    suli_i2c_init(this->i2c, pinsda, pinscl);
    on_power_on();
}

bool GroveAccMMA7660::on_power_on()
{
    _setmode(MMA7660_STAND_BY);
    _setsamplerate(AUTO_SLEEP_120);
    _setmode(MMA7660_ACTIVE);
    uint8_t r = (0x7 << 5);
    suli_i2c_write_reg(i2c, MMA7660_ADDR, MMA7660_INTSU, &r, 1); //enable shake detection on 3 axises.
    power_on = true;
    return true;
}

bool GroveAccMMA7660::on_power_off()
{
    power_on = false;
    return true;
}

void GroveAccMMA7660::_setmode(uint8_t mode)
{
    suli_i2c_write_reg(i2c, MMA7660_ADDR, MMA7660_MODE, &mode, 1);
}

void GroveAccMMA7660::_setsamplerate(uint8_t rate)
{
    suli_i2c_write_reg(i2c, MMA7660_ADDR, MMA7660_SR, &rate, 1);
}

void GroveAccMMA7660::_getxyz(int8_t *x, int8_t *y, int8_t *z)
{
    uint8_t val[3];

    do
    {
        suli_i2c_read_reg(i2c, MMA7660_ADDR, MMA7660_X, val, 3);
    }
    while (val[0]>63 || val[1]>63 || val[2]>63);

    //Serial1.println("raw: ");
    //Serial1.println(val[0]);
    //Serial1.println(val[1]);
    //Serial1.println(val[2]);

    *x = ((int8_t)(val[0] << 2)) / 4;
    *y = ((int8_t)(val[1] << 2)) / 4;
    *z = ((int8_t)(val[2] << 2)) / 4;
}

bool GroveAccMMA7660::read_acceleration(float *ax, float *ay, float *az)
{
    if (!power_on) 
    {
        *ax = 0;
        *ay = 0;
        *az = 0;
        return true;
    }
    
    int8_t x,y,z;
    _getxyz(&x,&y,&z);
    *ax = x/21.00;
    *ay = y/21.00;
    *az = z/21.00;

    return true;
}

bool GroveAccMMA7660::read_shaked(uint8_t *shaked)
{
    if (!power_on) 
    {
        *shaked = 0;
        return true;
    }
    
    uint8_t r;
    do
    {
        suli_i2c_read_reg(i2c, MMA7660_ADDR, MMA7660_TILT, &r, 1);
    } while ((r&0x40)>0);

    if (r&0x80)
    {
        *shaked = 1;
    } else
    {
        *shaked = 0;
    }
    return true;
}



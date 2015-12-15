/*
 * grove_acc_mma7660.h
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


#ifndef __GROVE_ACC_MMA7660_CLASS_H__
#define __GROVE_ACC_MMA7660_CLASS_H__

#include "suli2.h"

//GROVE_NAME        "Grove-3Axis Digital Acc(±1.5g)"
//SKU               101020039
//IF_TYPE           I2C
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/b/bb/3_aix_acc.jpg

#define MMA7660_ADDR  (0x4c<<1)

#define MMA7660_X     0x00
#define MMA7660_Y     0x01
#define MMA7660_Z     0x02
#define MMA7660_TILT  0x03
#define MMA7660_SRST  0x04
#define MMA7660_SPCNT 0x05
#define MMA7660_INTSU 0x06
#define MMA7660_MODE  0x07
#define MMA7660_STAND_BY 0x00
#define MMA7660_ACTIVE   0x01
#define MMA7660_SR    0x08      //sample rate register
#define AUTO_SLEEP_120  0X00    //120 sample per second
#define AUTO_SLEEP_64   0X01
#define AUTO_SLEEP_32   0X02
#define AUTO_SLEEP_16   0X03
#define AUTO_SLEEP_8    0X04
#define AUTO_SLEEP_4    0X05
#define AUTO_SLEEP_2    0X06
#define AUTO_SLEEP_1    0X07
#define MMA7660_PDET  0x09
#define MMA7660_PD    0x0A




class GroveAccMMA7660
{
public:
    GroveAccMMA7660(int pinsda, int pinscl);
    
    bool on_power_on();
    bool on_power_off();
    

    /**
     * read the acceleration in unit g
     *
     * @param ax - acceleration in x axis
     * @param ay - acceleration in y axis
     * @param az - acceleration in z axis
     *
     * @return bool
     */
    bool read_acceleration(float *ax, float *ay, float *az);

    /**
     * read the status if the thing was shacked
     * shake can be done in any axis
     *
     * @param shaked 1: shaked 0: not
     *
     * @return bool
     */
    bool read_shaked(uint8_t *shaked);

//private:
    I2C_T *i2c;
    bool power_on = true;

    void _setmode(uint8_t mode);
    void _setsamplerate(uint8_t rate);
    void _getxyz(int8_t *x, int8_t *y, int8_t *z);
};

#endif

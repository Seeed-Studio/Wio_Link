/*
 * grove_comp_hmc5883l.h
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



#ifndef __GROVE_COMPASS_CLASS_H__
#define __GROVE_COMPASS_CLASS_H__

#include "suli2.h"

//GROVE_NAME        "Grove-3Axis Compass"
//SKU               101020034
//IF_TYPE           I2C
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/thumb/b/be/Axis_compass.jpg/400px-Axis_compass.jpg

#define HMC5883L_ADDRESS (0x1E<<1)

#define CONFIGURATION_REGISTERA 0x00
#define CONFIGURATION_REGISTERB 0x01
#define MODE_REGISTER 0x02
#define DATA_REGISTER_BEGIN 0x03

#define MEASUREMENT_CONTINUOUS 0x00
#define MEASUREMENT_SINGLE_SHOT 0x01
#define MEASUREMENT_IDLE 0x03

#ifndef PI
#define PI ((float)3.1415926)
#endif

class GroveCompass
{
public:
    GroveCompass(int pinsda, int pinscl);
    
    /**
     * 
     * 
     * @param heading_deg - the angle of heading relative to the north, unit: degree
     * 
     * @return bool 
     */
    bool read_compass_heading(float *heading_deg);
private:
    I2C_T *i2c;
    uint8_t databuf[6];
    uint8_t cmdbuf[2];
    uint8_t mode;
};

#endif

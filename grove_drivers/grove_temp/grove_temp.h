/*
 * grove_temp.h
 *
 * Copyright (c) 2012 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Jack Shao (jacky.shaoxg@gmail.com)
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


#ifndef __GROVE_TEMP_SENSOR_H__
#define __GROVE_TEMP_SENSOR_H__

#include "suli2.h"

//GROVE_NAME        "Grove - Temperature Sensor"
//SKU               101020015
//IF_TYPE           ANALOG
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/5/5f/Grove_Temperature_Sensor_View.jpg


class GroveTemp
{
public:
    GroveTemp(int pin);
    
    /**
     * Read the celsius temperature from the temperature sensor.
     * 
     * @param temperature - celsius degree with an accuracy of 1.5°C .
     * 
     * @return bool 
     */
    bool read_temp(float *temperature);
private:
    ANALOG_T *io;
};


#endif

/*
 * grove_ultrasonic_ranger.h
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


#ifndef __GROVE_ULTRA_RANGER_H__
#define __GROVE_ULTRA_RANGER_H__

#include "suli2.h"

//GROVE_NAME        "Grove - Ultrasonic Ranger"
//SKU               101020010
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/thumb/3/3a/Ultrasonic_Ranger.jpg/350px-Ultrasonic_Ranger.jpg

class GroveUltraRanger
{
public:
    GroveUltraRanger(int pin);
    
    /**
     * Get the range / distance between sensor and object.
     * 
     * @param range_cm - unit: cm
     * 
     * @return bool 
     */
    bool read_range_in_cm(float *range_cm);
    
    /**
     * Get the range / distance between sensor and object.
     * 
     * @param range_cm - unit: inch
     * 
     * @return bool 
     */
    bool read_range_in_inch(float *range_inch);
    IO_T *io;
    
private:
    uint32_t _get_pulse_width();
};

#endif

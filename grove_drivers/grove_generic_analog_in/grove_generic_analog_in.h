/*
 * grove_generic_analog_in.h
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


#ifndef __GROVE_GENERIC_ANALOG_IN_H__
#define __GROVE_GENERIC_ANALOG_IN_H__

#include "suli2.h"

//GROVE_NAME        "Generic Analog Input"
//SKU               eedec01c-8c5a-11e5-8994-feff819cdc9f
//IF_TYPE           ANALOG
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/8/87/Pion_one_generic_analog.png


class GenericAIn
{
public:
    GenericAIn(int pin);
    
    /**
     * Read the ADC measurement of a generic analog device. The full range is 0~1023.
     * 
     * @param analog - The measurement of ADC, 0~1023
     * 
     * @return bool 
     */
    bool read_analog(int *analog);
private:
    ANALOG_T *io;
};


#endif

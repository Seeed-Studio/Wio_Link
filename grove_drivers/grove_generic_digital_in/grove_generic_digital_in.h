/*
 * grove_generic_digital_in.h
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


#ifndef __GROVE_GENERIC_DIGITAL_IN_H__
#define __GROVE_GENERIC_DIGITAL_IN_H__

#include "suli2.h"

//GROVE_NAME        "Generic Digital Input"
//SKU               7e3306bc-8911-11e5-af63-feff819cdc9f
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/e/ea/Pion_one_generic_din.png

class GenericDIn
{
public:
    GenericDIn(int pin);
    
    /**
     * Read the input state of a generic digital input device
     * 
     * @param input - 1: on, 0: off
     * 
     * @return bool 
     */
    bool read_input(uint8_t *input);
    
    /**
     * Event data is the number of the PIN to which the grove is attached
     * 
     * @param reporter 
     * 
     * @return EVENT_T* 
     */
    EVENT_T * attach_event_reporter_for_input_changed(EVENT_CALLBACK_T reporter);
    EVENT_T *event;
    IO_T *io;
    uint32_t time;
};

static void input_changed_interrupt_handler(void *para);

#endif

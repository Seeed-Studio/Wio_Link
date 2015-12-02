/*
 * grove_pir_motion.h
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


#ifndef __GROVE_PIR_MOTION_H__
#define __GROVE_PIR_MOTION_H__

#include "suli2.h"

//GROVE_NAME        "Grove-PIR Motion Sensor"
//SKU               101020020
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/depot/bmz_cache/b/bc0115270f17babe6373bce8ec1bfe8a.image.164x123.jpg

class GrovePIRMotion
{
public:
    GrovePIRMotion(int pin);
    
    /**
     * Detect the motion of IR object 
     * 
     * @param approach - 1: IR object approached, 0: no IR object approached
     * 
     * @return bool 
     */
    bool read_approach(uint8_t *approach);

    EVENT_T * attach_event_reporter_for_ir_approached(CALLBACK_T reporter);
    EVENT_T *event;
    IO_T *io;
    uint32_t time;
};

static void pir_motion_interrupt_handler(void *para);

#endif

/*
 * grove_speaker.h
 *
 * Copyright (c) 2012 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Jacky Zhang (qi.zhang@seeed.cc)
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


#ifndef __GROVE_SPEAKER_H__
#define __GROVE_SPEAKER_H__

#include "suli2.h"

//GROVE_NAME        "Grove-Speaker"
//SKU               107020001
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/depot/images/product/Grove%20Speaker_01.jpg


class GroveSpeaker
{
public:
    GroveSpeaker(int pin);
    
    /**
     * 
     * 
     * @param freq - Hz
     * @param duration_ms - a duration of x microseconds during which the speaker will produce the sound 
     * 
     * @return bool 
     */
    bool write_sound_ms(int freq, int duration_ms);
    
    /**
     * Start to produce sound with a specified frequency, and wont stop until you send the sound_stop command.
     * 
     * @param freq - Hz
     * 
     * @return bool 
     */
    bool write_sound_start(int freq);
    bool write_sound_stop();
private:
    PWM_T *io;
};


#endif

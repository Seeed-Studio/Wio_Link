/*
 * grove_speaker.cpp
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

#include "suli2.h"
#include "grove_speaker.h"

#if 0
static float pitches[14] = {
    /*a*/220.00,
    /*b*/246.94,
    /*c*/261.63,
    /*d*/293.66,
    /*e*/329.63,
    /*f*/349.23,
    /*g*/392.00,
    /*A*/440.00,
    /*B*/493.88,
    /*C*/523.25,
    /*D*/587.33,
    /*E*/659.26,
    /*F*/698.46,
    /*G*/783.99
};
#endif

GroveSpeaker::GroveSpeaker(int pin)
{
    this->io = (PWM_T *)malloc(sizeof(PWM_T));

    suli_pwm_init(this->io, pin);
    suli_pin_write(this->io, SULI_LOW);
}

//duration: the time sounds, unit: ms
//freq: the frequency of speaker, unit: Hz
bool GroveSpeaker::write_sound_ms(int freq, int duration_ms)
{  
    if(freq == 0 || duration_ms == 0) return false;
	
	uint32_t interval = (uint32_t)1000000 / freq;//convert the unit to us
    
    if (interval > 10000)
    {
        interval = 10000;
    } else if (interval < 5)
    {
        interval = 5;
    }

    uint32_t times = (uint32_t)duration_ms * 1000 / interval; //calcuate how many times the loop takes
    uint32_t times_5ms = 5000 / interval;
    
#if ENABLE_DEBUG_ON_UART1
	Serial1.print("interval");
	Serial1.println(interval);
	Serial1.print("times");
	Serial1.println(times);
#endif
    if (interval > 2000)
    {
        for (int i = 0; i < times; i++)
     	{
    		suli_pin_write(this->io, SULI_HIGH);
    		suli_delay_ms(interval/2000);
    		suli_pin_write(this->io, SULI_LOW);
    		suli_delay_ms(interval/2000);
        }
    } else
    {
        for (int i = 0; i < times; i++)
     	{
    		suli_pin_write(this->io, SULI_HIGH);
    		suli_delay_us(interval/2);
    		suli_pin_write(this->io, SULI_LOW);
    		suli_delay_us(interval/2);
            if (i % times_5ms == (times_5ms-1))
            {
                suli_delay_ms(0); //yield the cpu
            }
        }
    }
	
    return true;
}

bool GroveSpeaker::write_sound_start(int freq)
{
    suli_pwm_frequency(this->io, freq);
    suli_pwm_output(this->io, 50);
    return true;
}


bool GroveSpeaker::write_sound_stop()
{
    suli_pwm_output(this->io, 0);
    return true;
}

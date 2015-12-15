/*
 * grove_generic_pwm_out.cpp
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

#include "suli2.h"
#include "grove_generic_pwm_out.h"

GenericPWMOut::GenericPWMOut(int pin)
{
    this->io = (PWM_T *)malloc(sizeof(PWM_T));

    suli_pwm_init(this->io, pin);
    
    _freq = 1000;
    _percent = 0.0;
}

bool GenericPWMOut::write_pwm(float duty_percent)
{
    _percent = duty_percent;
    
    suli_pwm_output(io, duty_percent);
    
    return true;
}
bool GenericPWMOut::write_pwm_with_freq(float duty_percent, uint32_t freq)
{
    _freq = freq;
    _percent = duty_percent;
    
    suli_pwm_frequency(io, freq);
    suli_pwm_output(io, duty_percent);
    
    return true;
    
}
bool GenericPWMOut::read_pwm(float *duty_percent, uint32_t *freq)
{
    *duty_percent = _percent;
    *freq = _freq;
    
    return true;
}


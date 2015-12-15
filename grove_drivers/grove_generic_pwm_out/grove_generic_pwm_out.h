/*
 * grove_generic_pwm_out.h
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


#ifndef __GROVE_GENERIC_PWM_OUT_H__
#define __GROVE_GENERIC_PWM_OUT_H__

#include "suli2.h"

//GROVE_NAME        "Generic PWM/Analog Output"
//SKU               63e25800-a2d2-11e5-bf7f-feff819cdc9f
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/b/b4/Generic_analog_output.png


class GenericPWMOut
{
public:
    GenericPWMOut(int pin);
    
    /**
     * Output a PWM wave on specified IO. The default frequency is 1K Hz.
     * 
     * @param duty_percent - 0.0~100.0, float number
     * 
     * @return bool 
     */
    bool write_pwm(float duty_percent);
    
    /**
     * Output a PWM wave on speicfied IO with specified frequency.
     * 
     * @param duty_percent - 0.0~100.0, float number
     * @param freq - unit: Hz
     * 
     * @return bool 
     */
    bool write_pwm_with_freq(float duty_percent, uint32_t freq);
    
    /**
     * Read back the parameters of PWM.
     * 
     * @param duty_percent - 0.0~100.0, float number
     * @param freq - unit: Hz
     * 
     * @return bool 
     */
    bool read_pwm(float *duty_percent, uint32_t *freq);
    
    
private:
    PWM_T *io;
    uint32_t _freq;
    float _percent;
};


#endif

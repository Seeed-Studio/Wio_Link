/*
 * grove_ultrasonic_ranger.cpp
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
#include "grove_ultrasonic_ranger.h"



GroveUltraRanger::GroveUltraRanger(int pin)
{
    this->io = (IO_T *)malloc(sizeof(IO_T));

    suli_pin_init(io, pin, SULI_OUTPUT);
}

bool GroveUltraRanger::read_range_in_cm(float *range_cm)
{
    uint32_t d = _get_pulse_width();
    *range_cm = d / 29.4 / 2;
    return true;
}

bool GroveUltraRanger::read_range_in_inch(float *range_inch)
{
    uint32_t d = _get_pulse_width();
    *range_inch = d / 74.7 / 2;
    return true;
}

uint32_t GroveUltraRanger::_get_pulse_width()
{
    suli_pin_dir(io, SULI_OUTPUT);
    suli_pin_write(io, SULI_LOW);
    suli_delay_us(2);
    suli_pin_write(io, SULI_HIGH);
    suli_delay_us(5);
    suli_pin_write(io, SULI_LOW);
    suli_pin_dir(io, SULI_INPUT);
    return suli_pin_pulse_in(io, SULI_HIGH, 20000);
}

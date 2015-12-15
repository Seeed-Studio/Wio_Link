/*
 * grove_example.h
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
#include "grove_example.h"



GroveExample::GroveExample(int pin)
{
    this->io = (IO_T *)malloc(sizeof(IO_T));
    this->timer = (TIMER_T *)malloc(sizeof(TIMER_T));
    this->timer1 = (TIMER_T *)malloc(sizeof(TIMER_T));

    //suli_i2c_init(i2c, pinsda, pinscl);
    suli_pin_init(io, pin, SULI_INPUT);
    
    suli_timer_install(timer,  1000000, timer_handler, this, true);
    suli_timer_install(timer1, 1500000, timer1_handler, this, false);
}

bool GroveExample::read_temp(int *temp)
{
    *temp = 30;
    return true;
}

bool GroveExample::read_uint8_value(uint8_t *value)
{
    *value = 255;
    return true;
}

bool GroveExample::read_humidity(float *humidity)
{
    *humidity = 52.5;
    return true;
}

bool GroveExample::read_acc(float *ax, float *ay, float *az)
{
    *ax = 12.3; *ay = 45.6; *az = 78.0;
    return true;
}
bool GroveExample::read_compass(float *cx, float *cy, float *cz, int *degree)
{
    *cx = 12.3; *cy = 45.6; *cz = 78.0; *degree = 90;
    return true;
}

bool GroveExample::read_with_arg(float *cx, float *cy, float *cz, int *degree, int arg)
{
    *cx = 12.3; *cy = 45.6; *cz = 78.0; *degree = arg;
    return true;
}

bool GroveExample::write_acc_mode(uint8_t mode)
{
    //suli_i2c_write(i2c, 0x00, &mode, 1);
    return true;
}

bool GroveExample::write_float_value(float f)
{
    String str(f);
    Serial1.print("get float: ");
    Serial1.println(str);
    return false;
}

bool GroveExample::write_multi_value(int a, float b, uint32_t c)
{
    //_GroveExample::internal_function(i2c, b);
    Serial1.print("get uint32: ");
    Serial1.println(c);
    return true;
}


EVENT_T * GroveExample::attach_event_reporter_for_fire(EVENT_CALLBACK_T reporter)
{
    this->event1 = (EVENT_T *)malloc(sizeof(EVENT_T));

    suli_event_init(event1, reporter, NULL);

    suli_pin_attach_interrupt_handler(io, &pin_interrupt_handler, SULI_RISE, this);

    return this->event1;
}

void GroveExample::_internal_function(float x)
{

}

static void pin_interrupt_handler(void *para)
{
    GroveExample *g = (GroveExample *)para;

    suli_event_trigger(g->event1, (void *)(g->io), SULI_EDT_INT);
    
    g->var = 0;
    
    //suli_timer_remove(g->timer);
    //suli_timer_install(g->timer, 1000000, timer_handler, g, true);
    suli_timer_control_interval(g->timer, 2000000);
    suli_timer_control_interval(g->timer1, 2000000);
}

static void timer_handler(void *para)
{
    GroveExample *g = (GroveExample *)para;
    
    g->var++;
    
    digitalWrite(SWITCH_GROVE_POWER, ~digitalRead(SWITCH_GROVE_POWER));
    
    //Serial1.printf("example var: %d\r\n", g->var);
}

static void timer1_handler(void *para)
{
    GroveExample *g = (GroveExample *)para;
    
    g->var++;
    
    //Serial1.printf("example var 1: %d\r\n", g->var);
}





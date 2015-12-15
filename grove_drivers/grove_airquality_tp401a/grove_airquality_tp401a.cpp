/*
 * grove_airquality_tp401a.cpp
 *
 * Copyright (c) 2012 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Jacky Zhang
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


#include "grove_airquality_tp401a.h"

GroveAirquality::GroveAirquality(int pin)
{
    this->analog = (ANALOG_T *)malloc(sizeof(ANALOG_T));
    suli_analog_init(this->analog, pin);
    inited_time = suli_millis();
}

bool GroveAirquality::on_power_on()
{
    inited_time = suli_millis();
    return true;
}

bool GroveAirquality::on_power_off()
{
    return false;  //Air quality sensor will pull down VCC when power up, that will cause the system reboot unexpectedly.
}

bool GroveAirquality::read_quality(int *quality)
{        
    //judge if the sensor is prepared OK
    uint32_t now = suli_millis();
    if(now - inited_time < 20000)
    {
        error_desc = "the sensor is heating and can be read after 20s.";
        return false;
    }
    
    //get AD value
    *quality = suli_analog_read(analog);
    
    return true;
}


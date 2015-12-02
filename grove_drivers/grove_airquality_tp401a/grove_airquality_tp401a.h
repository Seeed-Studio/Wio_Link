/*
 * grove_airquality_tp401a.h
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



#ifndef __GROVE_AIRQUALITY_TP401A_H__
#define __GROVE_AIRQUALITY_TP401A_H__

#include "suli2.h"

//GROVE_NAME        "Grove - Air Quality Sensor"
//SKU               101020078
//IF_TYPE           ANALOG
//IMAGE_URL         http://www.seeedstudio.com/depot/images/product/101020078%201_02.jpg


class GroveAirquality
{
public:
    GroveAirquality(int pin);
    
    /**
     * Get the analog reading of air quality. The air quality sensor is is designed for comprehensive 
     * monitor over indoor air condition. It's responsive to a wide scope of harmful gases, as carbon 
     * monixide, alcohol, acetone, thinner, formaldehyde and so on. Due to the measuring mechanism, this 
     * sensor can not output specific data to describe target gases' concentrations quantitatively. But 
     * it's still competent enough to be used in applications that require only qualitative results, 
     * like auto refresher sprayers and auto air cycling systems. If exposed for a long time to pollution, 
     * it will be damaged.
     * 
     * @param quality - analog reading of this sensor, please use the data as a comparative	quantity, because it is a qualitative results
     * 
     * @return bool 
     */
    bool read_quality(int *quality);
    char *get_last_error() { return error_desc; };

private:
    ANALOG_T *analog;
    uint32_t inited_time;
    char *error_desc;
};

#endif

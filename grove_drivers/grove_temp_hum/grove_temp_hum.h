/*
 * grove_temp_hum.h
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



#ifndef __GROVE_TEMP_HUM_CLASS_H__
#define __GROVE_TEMP_HUM_CLASS_H__

#include "suli2.h"

//GROVE_NAME        "Grove-Temperature&Humidity"
//SKU               101020019-ffff
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/3/36/Temp%26Humi.jpg

#define MAXTIMINGS 85
#define DHT11 11
#define DHT22 22
#define DHT21 21
#define AM2301 21

#ifdef ESP8266
#define PULSE_COUNTER              20
#elif defined(__MBED__)
#define PULSE_COUNTER              20
#else
#define PULSE_COUNTER              6
#endif


class GroveTempHum
{
public:
    GroveTempHum(int pin);
    
    /**
     * The basic temperature and humidity sensor will update the reading every 2 seconds.
     * 
     * @param celsius_degree - unit: Celsius degree
     * 
     * @return bool 
     */
    bool read_temperature(float *celsius_degree);
    
    /**
     * The basic temperature and humidity sensor will update the reading every 2 seconds.
     * 
     * @param fahrenheit_degree - Fahrenheit degree
     * 
     * @return bool 
     */
    bool read_temperature_f(float *fahrenheit_degree);
    
    /**
     * The basic temperature and humidity sensor will update the reading every 2 seconds.
     * 
     * @param humidity - 0~100(%)
     * 
     * @return bool 
     */
    bool read_humidity(float *humidity);

private:
    IO_T *io;
    bool _read(IO_T *io);
    float _convertCtoF(float c);
    uint8_t _type;
    uint8_t _count;
    bool firstreading;
    unsigned long _lastreadtime;
    uint8_t data[6];

};

#endif


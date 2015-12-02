/*
 * grove_temp_hum.cpp
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

#include "suli2.h"
#include "grove_temp_hum.h"


GroveTempHum::GroveTempHum(int pin)
{
    this->io = (IO_T *)malloc(sizeof(IO_T));
    suli_pin_init(io, pin, SULI_INPUT);
    _type = DHT11;
    _count = PULSE_COUNTER;
    firstreading = true;
}

bool GroveTempHum::_read(IO_T *io)
{
    uint8_t laststate = SULI_HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    unsigned long currenttime;

    // pull the pin high and wait 250 milliseconds
    //digitalWrite(_pin, SULI_HIGH);
    suli_pin_write(io, SULI_HIGH);
    suli_delay_ms(250);

    currenttime = suli_millis();
    if (currenttime < _lastreadtime)
    {
        // ie there was a rollover
        _lastreadtime = 0;
    }
    if (!firstreading && ((currenttime - _lastreadtime) < 2000))
    {
        return true; // return last correct measurement
                     //delay(2000 - (currenttime - _lastreadtime));
    }
    firstreading = false;

    _lastreadtime = suli_millis();

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    // now pull it low for ~20 milliseconds
    //pinMode(_pin, OUTPUT);
    suli_pin_dir(io, SULI_OUTPUT);
    //digitalWrite(_pin, LOW);
    suli_pin_write(io, SULI_LOW);
    suli_delay_ms(20);
    //cli();
    //digitalWrite(_pin, SULI_HIGH);
    suli_pin_write(io, SULI_HIGH);
    //delayMicroseconds(40);
    suli_delay_us(40);
    //pinMode(_pin, INPUT);
    suli_pin_dir(io, SULI_INPUT);
    // read in timings
    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        //while (digitalRead(_pin) == laststate) {
        while (suli_pin_read(io) == laststate)
        {
            counter++;
            //delayMicroseconds(1);
            suli_delay_us(1);
            if (counter == 255)
            {
                break;
            }
        }
        //laststate = digitalRead(&_pin);
        laststate = suli_pin_read(io);

        if (counter == 255) break;

        // ignore first 3 transitions
        if ((i >= 4) && (i % 2 == 0))
        {
            // shove each bit into the storage bytes
            data[j / 8] <<= 1;
            if (counter > _count) //
                data[j / 8] |= 1;
            j++;
        }

    }

    // check we read 40 bits and that the checksum matches
    if ((j >= 40) &&
        (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)))
    {
        return true;
    }


    return false;

}

float GroveTempHum::_convertCtoF(float c)
{
    return c * 9 / 5 + 32;
}

//boolean S == Scale.  True == Farenheit; False == Celcius
bool GroveTempHum::read_temperature(float *temperature)
{
    float f;

    if (_read(io))
    {
        switch (_type)
        {
        case DHT11:
            f = data[2];
            *temperature = f;
            return true;
        case DHT22:
        case DHT21:
            f = data[2] & 0x7F;
            f *= 256;
            f += data[3];
            f /= 10;
            if (data[2] & 0x80) f *= -1;
            *temperature = f;
            return true;
        }
    }
    //Serial.print("Read fail");
    return false;
}

bool GroveTempHum::read_temperature_f(float *temperature)
{
    float t;
    if (!read_temperature(&t)) return false;
    *temperature = _convertCtoF(t);
    return true;
}

bool GroveTempHum::read_humidity(float *humidity)
{
    float f;
    if (_read(io))
    {
        switch (_type)
        {
        case DHT11:
            f = data[0];
            *humidity = f;
            return true;
        case DHT22:
        case DHT21:
            f = data[0];
            f *= 256;
            f += data[1];
            f /= 10;
            *humidity = f;
            return true;
        }
    }
    //Serial.print("Read fail");
    return false;
}


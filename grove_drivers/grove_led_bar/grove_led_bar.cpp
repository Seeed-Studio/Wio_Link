/*
 * grove_led_bar.cpp
 *
 * Copyright (c) 2012 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Jack Shao
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
#include "grove_led_bar.h"

GroveLEDBar::GroveLEDBar(int pintx, int pinrx)
{
    this->io_clk = (IO_T *)malloc(sizeof(IO_T));
    this->io_data = (IO_T *)malloc(sizeof(IO_T));
    
    __greenToRed = 1;
        
    for (byte i = 0; i < 10; i++)
        __state[i] = 0x00;  // persist state so individual leds can be toggled

    suli_pin_init(io_clk, pintx, SULI_OUTPUT);
    suli_pin_init(io_data, pinrx, SULI_OUTPUT);
}

void GroveLEDBar::sendData(uint16_t data)
{
    for (uint8_t  i = 0; i < 16; i++)
    {
        uint16_t state = (data & 0x8000) ? SULI_HIGH : SULI_LOW;
        suli_pin_write(io_data, state);

        state = suli_pin_read(io_clk) ? SULI_LOW : SULI_HIGH;
        suli_pin_write(io_clk, state);

        data <<= 1;
    }
}

void GroveLEDBar::latchData(void)
{
    suli_pin_write(io_data, SULI_LOW);
    suli_delay_us(10);

    for (uint8_t  i = 0; i < 4; i++)
    {
        suli_pin_write(io_data, SULI_HIGH);
        suli_pin_write(io_data, SULI_LOW);
    }
}

void GroveLEDBar::setData(uint8_t  bits[])
{
    sendData(GLB_CMDMODE);

    for (uint8_t  i = 0; i < 10; i++)
    {
        if (__greenToRed)
        {
            // Go backward on __state
            sendData(__state[10 - i - 1]);
        } else
        {
            // Go forward on __state
            sendData(__state[i]);
        }
    }

    // Two extra empty bits for padding the command to the correct length
    sendData(0x00);
    sendData(0x00);

    latchData();
}

bool GroveLEDBar::write_orientation(uint8_t green_to_red)
{
    __greenToRed = green_to_red;

    setData(__state);
    return true;
}


bool GroveLEDBar::write_level(float level)
{
    level = max(0, min(10, level));
    level *= 8; // there are 8 (noticable) levels of brightness on each segment

    // Place number of 'level' of 1-bits on __state
    for (byte i = 0; i < 10; i++)
    {
        __state[i] = (level > 8) ? ~0 :
                                   (level > 0) ? ~(~0 << byte(level)) : 0;

        level -= 8;
    };

    setData(__state);
    return true;
}


bool GroveLEDBar::write_single_led(uint8_t led, float brightness)
{
    led = max(1, min(10, led));
    brightness = max(0, min(brightness, 1));

    // Zero based index 0-9 for bitwise operations
    led--;

    // 8 (noticable) levels of brightness
    // 00000000 darkest
    // 00000011 brighter
    // ........
    // 11111111 brightest
    __state[led] = ~(~0 << (uint8_t)(brightness * 8));

    setData(__state);
    return true;
}


bool GroveLEDBar::write_toggle(uint8_t led)
{
    led = max(1, min(10, led));

    // Zero based index 0-9 for bitwise operations
    led--;

    __state[led] = __state[led] ? 0 : ~0;

    setData(__state);
    return true;
}


bool GroveLEDBar::write_bits(uint16_t bits)
{
    for (uint8_t  i = 0; i < 10; i++)
    {

        if ((bits % 2) == 1) __state[i] = 0xFF;
        else __state[i] = 0x00;
        bits /= 2;
    }

    setData(__state);
    return true;
}


bool GroveLEDBar::read_bits(uint16_t *bits)
{
    uint16_t __bits = 0x00;
    for (uint8_t  i = 0; i < 10; i++)
    {
        if (__state[i] != 0x0) __bits |= (0x1 << i);
    }
    *bits = __bits;
    return true;
}

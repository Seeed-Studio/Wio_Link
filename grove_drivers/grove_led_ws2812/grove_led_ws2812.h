/*
 * grove_led_ws2812.h
 *
 * Copyright (c) 2015 seeed technology inc.
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
 *
 *
 */

/*
 * +++++++++++++++++++++++++++++++++
 * NOTE:
 * This library is only for esp8266
 * +++++++++++++++++++++++++++++++++
 */


#ifndef __GROVE_LED_WS2812_H__
#define __GROVE_LED_WS2812_H__

#include "suli2.h"

//GROVE_NAME        "Grove-WS2812 LED Strip 60"
//SKU               104990089
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/depot/bmz_cache/4/4f346dc15724a7b5a5c1383253aeefc9.image.530x397.jpg

#define MAX_LED_CNT             60

struct rgb_s
{
    uint8_t g,r,b;
}__attribute__((__packed__));

union rgb_buffer_u
{
    uint8_t buff[MAX_LED_CNT*3];
    struct rgb_s pixels[MAX_LED_CNT];
};

class GroveLedWs2812
{
public:
    GroveLedWs2812(int pin);
    char *get_last_error() { return error_desc; };
    
    /**
     * Set or reset or clear the led strip with a specified color.
     * 
     * @param total_led_cnt - the total count of this strip, max: 60(too many leds will cause the power unstable)
     * @param rgb_hex_string - a rgb value in hex format, e.g. AA55CC (without # or 0x)
     * 
     * @return bool 
     */
    bool write_clear(uint8_t total_led_cnt, char *rgb_hex_string);
    
    /**
     * Change the color of some piece of segment of the led strip.
     * We need to specify a list of rgb hex value concatinated into a string.
     * The segment will be defined with a start index and the length.
     * The length equals rgb_hex_string's length / 6. 
     * 
     * @param start - the start index of the segment(included)
     * @param rgb_hex_string - a list of rgb hex value, e.g. FFFFFFBBBBBBCCCCCC000000111111, max length: 240 or 40 rgb hex
     * 
     * @return bool 
     */
    bool write_segment(uint8_t start, char *rgb_hex_string);

private:
    IO_T *io;
    union rgb_buffer_u rgb_buffer;
    bool _extract_rgb_from_string(int start, char *str);
    char *error_desc;
    uint8_t led_cnt;
};

#endif


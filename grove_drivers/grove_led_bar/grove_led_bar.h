/*
 * grove_led_bar.h
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


#ifndef __GROVE_LED_BAR_H__
#define __GROVE_LED_BAR_H__

#include "suli2.h"

//GROVE_NAME        "Grove - LED Bar"
//SKU               104020006
//IF_TYPE           UART
//IMAGE_URL         http://www.seeedstudio.com/depot/images/product/104020006%201.jpg

// Avoid name conflict
#define GLB_CMDMODE 0x00  // Work on 8-bit mode
#define GLB_ON      0xff  // 8-bit 1 data
#define GLB_OFF     0x00  // 8-bit 0 data

class GroveLEDBar
{
public:

    GroveLEDBar(int pintx, int pinrx);

    /**
     * Change the orientation of the level display
     * 
     * @param green_to_red - 1: green to red 0: red to green
     * 
     * @return bool 
     */
    bool write_orientation(uint8_t green_to_red);

    /**
     * Display a level
     * 
     * @param level - 0~10, the decimal part will control the brightness of the last led, e.g. 9.5 will cause the 10th led light up in half brightness.
     * 
     * @return bool 
     */
    bool write_level(float level);

    /**
     * Control a single led with brightness
     * 
     * @param led - the index, 1~10
     * @param brightness - 0.0~1.0, 1.0 is the brightest, 0.0 let this led off
     * 
     * @return bool 
     */
    bool write_single_led(uint8_t led, float brightness);

    /**
     * Toggle a single led on or off.
     * 
     * @param led - the index, 1~10
     * 
     * @return bool 
     */
    bool write_toggle(uint8_t led);
    
    /**
     * Control the leds with the bit mask of a uint16_t integer number
     * 
     * @param bits - each bit controls the led in that bit mask, bit value 0: off, bit value 1: on
     * 
     * @return bool 
     */
    bool write_bits(uint16_t bits);
    
    /**
     * Read the states of each led, return bit mask of a uint16_t integer number.
     * 
     * @param bits - each bit indicates the led status in that bit mask, bit value 0: off, bit value 1: on
     * 
     * @return bool 
     */
    bool read_bits(uint16_t *bits);

private:

    IO_T *io_clk;  // Clock pin
    IO_T *io_data;   // Data pin
    bool __greenToRed;        // Orientation (0 = red to green, 1 = green to red)
    uint8_t __state[10]; // Led state, brightness for each LED

    void sendData(uint16_t data);  // Send a word to led bar
    void latchData(void);              // Load data into the latch register
    void setData(uint8_t  bits[]); //Set data array

};

#endif

/*
 * grove_4_digit.h
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


#ifndef __GROVE_4_DIGIT_H__
#define __GROVE_4_DIGIT_H__

#include "suli2.h"

//GROVE_NAME        "Grove - 4-Digit Display"
//SKU               104030003
//IF_TYPE           UART
//IMAGE_URL         http://www.seeedstudio.com/depot/bmz_cache/3/3a9f79323a82950c12fc7e69fa9fab4d.image.530x397.jpg

// Avoid name conflict
#define GLB_CMDMODE 0x00  // Work on 8-bit mode
#define GLB_ON      0xff  // 8-bit 1 data
#define GLB_OFF     0x00  // 8-bit 0 data

class Grove4Digit
{
public:

    Grove4Digit(int pintx, int pinrx);
    
    bool on_power_on();
    bool on_power_off();
    
    /**
     * Display the ":" point or not
     * 
     * @param display - 0: not, 1: display
     * 
     * @return bool 
     */
    bool write_display_point(uint8_t display);
    
    /**
     * Display one digit. 
     * 
     * @param position - 0~3, 0 is the most left
     * @param chr - the character, can be 0~9,A,b,C,d,E,F,V,U.
     * 
     * @return bool 
     */
    bool write_display_one_digit(uint8_t position, char *chr);
    
    /**
     * Display multiple digits.
     * 
     * @param start_pos - 0~3, 0 is the most left
     * @param chars - the characters, can be 0~9,A,b,C,d,E,F,V,U.
     * 
     * @return bool 
     */
    bool write_display_digits(uint8_t start_pos, char *chars);
    
    /**
     * Clear all digits and ":" point.
     * 
     * @return bool 
     */
    bool write_clear();
    
    /**
     * Set the brightness.
     * 
     * @param brightness - can be 0,2,7, 7 is the brightest.
     * 
     * @return bool 
     */
    bool write_brightness(uint8_t brightness);
    
    char *get_last_error() { return error_desc; };

private:

    IO_T *io_clk;  // Clock pin
    IO_T *io_data;   // Data pin
    uint8_t Cmd_DispCtrl;        
    uint8_t _PointFlag;     //_PointFlag=1:the clock point on
    uint8_t data[5];
    char *error_desc;

    void writeByte(uint8_t wr_data);
    void start(void);
    void stop(void);
    uint8_t coding(uint8_t DispData);
    void codings(uint8_t *data_in, int len, uint8_t *data_out);
    void refresh();

};

#endif

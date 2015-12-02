/*
 * grove_oled_12864.h
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


#ifndef __OLED_128x64_SULI_H__
#define __OLED_128x64_SULI_H__

#include "suli2.h"

//GROVE_NAME        "Grove - OLED Display 1.12''"
//SKU               104030008
//IF_TYPE           I2C
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/thumb/e/ea/Toled128642.jpg/400px-Toled128642.jpg



#define SeeedOLED_Max_X 		        127	//128 Pixels
#define SeeedOLED_Max_Y 		        63	//64  Pixels

#define PAGE_MODE			            01
#define HORIZONTAL_MODE			        02

#define SeeedOLED_Address		        (0x3c<<1)
#define SeeedOLED_Command_Mode		    0x80
#define SeeedOLED_Data_Mode		        0x40
#define SeeedOLED_Display_Off_Cmd	    0xAE
#define SeeedOLED_Display_On_Cmd	    0xAF
#define SeeedOLED_Normal_Display_Cmd	0xA6
#define SeeedOLED_Inverse_Display_Cmd	0xA7
#define SeeedOLED_Activate_Scroll_Cmd	0x2F
#define SeeedOLED_Dectivate_Scroll_Cmd	0x2E
#define SeeedOLED_Set_Brightness_Cmd	0x81

#define Scroll_Left			            0x00
#define Scroll_Right			        0x01

#define Scroll_2Frames			        0x7
#define Scroll_3Frames			        0x4
#define Scroll_4Frames			        0x5
#define Scroll_5Frames			        0x0
#define Scroll_25Frames			        0x6
#define Scroll_64Frames			        0x1
#define Scroll_128Frames		        0x2
#define Scroll_256Frames		        0x3




class GroveOLED12864
{
public:
    GroveOLED12864(int pinsda, int pinscl);

    /**
     * Clear the whole screen
     * 
     * @return bool 
     */
    bool write_clear();
    
    /**
     * Set the brightness for the screen
     * 
     * @param brightness - 0~255
     * 
     * @return bool 
     */
    bool write_brightness(uint8_t brightness);
    
    /**
     * Print a integer
     * 
     * @param row - 0~7
     * @param col - 0~15
     * @param i - the integer to display
     * 
     * @return bool 
     */
    bool write_integer(uint8_t row, uint8_t col, int32_t i);
    
    /**
     * Print a float number
     * 
     * @param row - 0~7
     * @param col - 0~15
     * @param f - float number
     * @param decimal - the decimal bits
     * 
     * @return bool 
     */
    bool write_float(uint8_t row, uint8_t col, float f, uint8_t decimal);
    
    /**
     * Print a string in one line.
     * Note that the char must be letter or number, special chars may be ignored. 
     * To display multilines and special chars, please use base64_string API.
     * 
     * @param row - 0~7
     * @param col - 0~15
     * @param str - the string to display
     * 
     * @return bool 
     */
    bool write_string(uint8_t row, uint8_t col, char *str);
    
    /**
     * Print a multiline string encoded in base64. 
     * Special chars is supported. 
     * 
     * @param row - 0~7
     * @param col - 0~15
     * @param b64_str - base64 encoded string, the max length is 256
     * 
     * @return bool 
     */
    bool write_base64_string(uint8_t row, uint8_t col, char *b64_str);
    
    /**
     * Let the specified rows to scroll left horizontally
     * 
     * @param start_row - upper row index, 0~7
     * @param end_row - lower row index (closed range), 0~7, must be greater or equal start_row.
     * @param speed - 0~255
     * 
     * @return bool 
     */
    bool write_scroll_left(uint8_t start_row, uint8_t end_row, uint8_t speed);
    
    /**
     * Let the specified rows to scroll right horizontally
     * 
     * @param start_row - upper row index, 0~7
     * @param end_row - lower row index (closed range), 0~7, must be greater or equal start_row.
     * @param speed - 0~255
     * 
     * @return bool 
     */
    bool write_scroll_right(uint8_t start_row, uint8_t end_row, uint8_t speed);
    
    /**
     * Stop all scrolling rows.
     * 
     * @return bool 
     */
    bool write_stop_scroll();
    
    /**
     * Set the screen to display in inverse mode.
     * 
     * @param inverse_or_not - 1: inverse(white background), 0: normal(black background)
     * 
     * @return bool 
     */
    bool write_inverse_display(uint8_t inverse_or_not);
    
    char *get_last_error() { return error_desc; };
    

private:
    I2C_T *i2c;
    
    uint8 addressingMode = PAGE_MODE;
    char *error_desc;
    int last_row;

    // send command
    void oled_128x64_cmd(uint8_t command);
    // send data
    void oled_128x64_dta(uint8_t data);
    // horizontal mode
    void oled_128x64_hmode();
    // page mode
    void oled_128x64_pagemode();
    
    void oled_128x64_XY(uint8_t row, uint8_t column);
    
    void oled_128x64_char(uint8_t c);
    
    uint8_t oled_128x64_num(int32_t long_num);
    
    void oled_128x64_string(const char *str);
    
    void oled_128x64_hsp(bool direction, uint8_t startPage, uint8_t endPage, uint8_t scrollSpeed);
    
    void oled_128x64_active_scroll();
    
    void oled_128x64_deactive_scroll();
    
    void oled_128x64_normal_display();
    
    void oled_128x64_inversel_display();
};

#endif

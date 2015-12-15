/*
 * grove_4_digit.cpp
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
#include "grove_4_digit.h"


//************definitions for TM1637*********************
#define ADDR_AUTO  0x40
#define ADDR_FIXED 0x44

#define STARTADDR  0xc0 
/**** definitions for the clock point of the digit tube *******/
#define POINT_ON   1
#define POINT_OFF  0




const int8_t TubeTab[] =
{
    0x3f, 0x06, 0x5b, 0x4f,
    0x66, 0x6d, 0x7d, 0x07,
    0x7f, 0x6f, 0x77, 0x7c,
    0x39, 0x5e, 0x79, 0x71,
    0x3e
}; //0~9,A,b,C,d,E,F



Grove4Digit::Grove4Digit(int pintx, int pinrx)
{
    this->io_clk = (IO_T *)malloc(sizeof(IO_T));
    this->io_data = (IO_T *)malloc(sizeof(IO_T));

    suli_pin_init(io_clk, pinrx, SULI_OUTPUT);
    suli_pin_init(io_data, pintx, SULI_OUTPUT);

    Cmd_DispCtrl = 0x88 + 7;   //7 is brightest
    
    write_clear();
}

bool Grove4Digit::on_power_on()
{
    refresh();
    return true;
}

bool Grove4Digit::on_power_off()
{
    suli_pin_write(io_clk, SULI_LOW);
    suli_pin_write(io_data, SULI_LOW);
    return true;
}


void Grove4Digit::writeByte(uint8_t wr_data)
{
    uint8_t i, count1;
    uint16_t count2=0;
    for (i = 0; i < 8; i++)                                  //sent 8bit data
    {
        suli_pin_write(io_clk, SULI_LOW);
        if (wr_data & 0x01) suli_pin_write(io_data, SULI_HIGH); //LSB first
        else suli_pin_write(io_data, SULI_LOW);
        wr_data >>= 1;
        suli_pin_write(io_clk, SULI_HIGH);
    }
    suli_pin_write(io_clk, SULI_LOW);                 //wait for the ACK
    suli_pin_write(io_data, SULI_HIGH);
    suli_pin_write(io_clk, SULI_HIGH);

    suli_pin_dir(io_data, SULI_INPUT);
    while (suli_pin_read(io_data))
    {
        count1 += 1;
        count2++;
        if (count1 == 200) //
        {
            suli_pin_dir(io_data, SULI_OUTPUT);
            suli_pin_write(io_data, SULI_LOW);
            count1 = 0;
        }
        suli_pin_dir(io_data, SULI_INPUT);
        if (count2 > 10000) return;
    }
    suli_pin_dir(io_data, SULI_OUTPUT);
}


void Grove4Digit::start(void)
{
    suli_pin_write(io_clk, SULI_HIGH); //send start signal to TM1637
    suli_pin_write(io_data, SULI_HIGH);
    suli_pin_write(io_data, SULI_LOW);
    suli_pin_write(io_clk, SULI_LOW);
}
void Grove4Digit::stop(void)
{
    suli_pin_write(io_clk, SULI_LOW);
    suli_pin_write(io_data, SULI_LOW);
    suli_pin_write(io_clk, SULI_HIGH);
    suli_pin_write(io_data, SULI_HIGH);
}
uint8_t Grove4Digit::coding(uint8_t DispData)
{
    uint8_t PointData;
    if (_PointFlag == POINT_ON) PointData = 0x80;
    else PointData = 0;
    if (DispData == 0x7f)
    {
        DispData = 0x00 + PointData; //The bit digital tube off
    }
    else if (DispData >= 0x30 && DispData <= 0x39)
    {
        DispData = TubeTab[DispData & 0x0f] + PointData;
    } else if (DispData == 'A')
    {
        DispData = TubeTab[0xa] + PointData;
    }
    else if (DispData == 'b')
    {
        DispData = TubeTab[0xb] + PointData;
    }
    else if (DispData == 'C')
    {
        DispData = TubeTab[0xc] + PointData;
    }
    else if (DispData == 'd')
    {
        DispData = TubeTab[0xd] + PointData;
    }
    else if (DispData == 'E')
    {
        DispData = TubeTab[0xe] + PointData;
    }
    else if (DispData == 'F')
    {
        DispData = TubeTab[0xf] + PointData;
    }
    else if (DispData == 'V' || DispData == 'U')
    {
        DispData = TubeTab[0x10] + PointData;
    }
    else
    {
        DispData = 0x00 + PointData;
    }
    return DispData;
}
void Grove4Digit::codings(uint8_t *data_in, int len, uint8_t *data_out)
{
    for(uint8_t i = 0;i < len;i ++)
    {
        data_out[i] = coding(data_in[i]);
    }
}

void Grove4Digit::refresh()
{
    uint8_t SegData[4];
    uint8_t i;
        
    codings(data, 4, SegData);
    start();          //start signal sent to TM1637 from MCU
    writeByte(ADDR_AUTO); //
    stop();           //
    start();          //
    writeByte(0xc0); //
    for (i = 0; i < 4; i++)
    {
        writeByte(SegData[i]);        //
    }
    stop();           //
    start();          //
    writeByte(Cmd_DispCtrl); //
    stop();     
    
}

bool Grove4Digit::write_display_point(uint8_t display)
{
    _PointFlag = display;
    refresh();
    
    return true;
}


bool Grove4Digit::write_display_one_digit(uint8_t position, char *chr)
{
    uint8_t SegData;
    
    position = constrain(position, 0, 3);
    
    data[position] = chr[0];
    
    SegData = coding(chr[0]);
    start();          //start signal sent to TM1637 from MCU
    writeByte(ADDR_FIXED); //
    stop();           //
    start();          //
    writeByte(position | 0xc0); //
    writeByte(SegData); //
    stop();            //
    start();          //
    writeByte(Cmd_DispCtrl); //
    stop();           //
    return true;
}

bool Grove4Digit::write_display_digits(uint8_t start_pos, char *chars)
{
    if (start_pos > 3)
    {
        error_desc = "position should in 0~3";
        return false;
    }
    uint8_t len = strlen(chars);
    for (uint8_t i = 0; i < len && (start_pos+i) < 4; i++)
    {
        data[start_pos+i] = chars[i];
    }
    refresh();
    return true;
}


bool Grove4Digit::write_clear()
{
    memset(data, 0x7f, 4);
    _PointFlag = 0;
    refresh();
    
    return true;
}

bool Grove4Digit::write_brightness(uint8_t brightness)
{
    Cmd_DispCtrl = 0x88 + brightness;
    refresh();
    
    return true;
}




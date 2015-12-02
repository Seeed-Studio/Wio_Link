/*
 * grove_comp_hmc5883l.cpp
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


#include "grove_gesture_paj7620.h"

#define USE_DEBUG (1)

// PAJ7620U2_20140305.asc
unsigned char init_register_array[][2] = {	// Initial Gesture
	{0xEF,0x00},
	{0x32,0x29},
	{0x33,0x01},
	{0x34,0x00},
	{0x35,0x01},
	{0x36,0x00},
	{0x37,0x07},
	{0x38,0x17},
	{0x39,0x06},
	{0x3A,0x12},
	{0x3F,0x00},
	{0x40,0x02},
	{0x41,0xFF},
	{0x42,0x01},
	{0x46,0x2D},
	{0x47,0x0F},
	{0x48,0x3C},
	{0x49,0x00},
	{0x4A,0x1E},
	{0x4B,0x00},
	{0x4C,0x20},
	{0x4D,0x00},
	{0x4E,0x1A},
	{0x4F,0x14},
	{0x50,0x00},
	{0x51,0x10},
	{0x52,0x00},
	{0x5C,0x02},
	{0x5D,0x00},
	{0x5E,0x10},
	{0x5F,0x3F},
	{0x60,0x27},
	{0x61,0x28},
	{0x62,0x00},
	{0x63,0x03},
	{0x64,0xF7},
	{0x65,0x03},
	{0x66,0xD9},
	{0x67,0x03},
	{0x68,0x01},
	{0x69,0xC8},
	{0x6A,0x40},
	{0x6D,0x04},
	{0x6E,0x00},
	{0x6F,0x00},
	{0x70,0x80},
	{0x71,0x00},
	{0x72,0x00},
	{0x73,0x00},
	{0x74,0xF0},
	{0x75,0x00},
	{0x80,0x42},
	{0x81,0x44},
	{0x82,0x04},
	{0x83,0x20},
	{0x84,0x20},
	{0x85,0x00},
	{0x86,0x10},
	{0x87,0x00},
	{0x88,0x05},
	{0x89,0x18},
	{0x8A,0x10},
	{0x8B,0x01},
	{0x8C,0x37},
	{0x8D,0x00},
	{0x8E,0xF0},
	{0x8F,0x81},
	{0x90,0x06},
	{0x91,0x06},
	{0x92,0x1E},
	{0x93,0x0D},
	{0x94,0x0A},
	{0x95,0x0A},
	{0x96,0x0C},
	{0x97,0x05},
	{0x98,0x0A},
	{0x99,0x41},
	{0x9A,0x14},
	{0x9B,0x0A},
	{0x9C,0x3F},
	{0x9D,0x33},
	{0x9E,0xAE},
	{0x9F,0xF9},
	{0xA0,0x48},
	{0xA1,0x13},
	{0xA2,0x10},
	{0xA3,0x08},
	{0xA4,0x30},
	{0xA5,0x19},
	{0xA6,0x10},
	{0xA7,0x08},
	{0xA8,0x24},
	{0xA9,0x04},
	{0xAA,0x1E},
	{0xAB,0x1E},
	{0xCC,0x19},
	{0xCD,0x0B},
	{0xCE,0x13},
	{0xCF,0x64},
	{0xD0,0x21},
	{0xD1,0x0F},
	{0xD2,0x88},
	{0xE0,0x01},
	{0xE1,0x04},
	{0xE2,0x41},
	{0xE3,0xD6},
	{0xE4,0x00},
	{0xE5,0x0C},
	{0xE6,0x0A},
	{0xE7,0x00},
	{0xE8,0x00},
	{0xE9,0x00},
	{0xEE,0x07},
	{0xEF,0x01},
	{0x00,0x1E},
	{0x01,0x1E},
	{0x02,0x0F},
	{0x03,0x10},
	{0x04,0x02},
	{0x05,0x00},
	{0x06,0xB0},
	{0x07,0x04},
	{0x08,0x0D},
	{0x09,0x0E},
	{0x0A,0x9C},
	{0x0B,0x04},
	{0x0C,0x05},
	{0x0D,0x0F},
	{0x0E,0x02},
	{0x0F,0x12},
	{0x10,0x02},
	{0x11,0x02},
	{0x12,0x00},
	{0x13,0x01},
	{0x14,0x05},
	{0x15,0x07},
	{0x16,0x05},
	{0x17,0x07},
	{0x18,0x01},
	{0x19,0x04},
	{0x1A,0x05},
	{0x1B,0x0C},
	{0x1C,0x2A},
	{0x1D,0x01},
	{0x1E,0x00},
	{0x21,0x00},
	{0x22,0x00},
	{0x23,0x00},
	{0x25,0x01},
	{0x26,0x00},
	{0x27,0x39},
	{0x28,0x7F},
	{0x29,0x08},
	{0x30,0x03},
	{0x31,0x00},
	{0x32,0x1A},
	{0x33,0x1A},
	{0x34,0x07},
	{0x35,0x07},
	{0x36,0x01},
	{0x37,0xFF},
	{0x38,0x36},
	{0x39,0x07},
	{0x3A,0x00},
	{0x3E,0xFF},
	{0x3F,0x00},
	{0x40,0x77},
	{0x41,0x40},
	{0x42,0x00},
	{0x43,0x30},
	{0x44,0xA0},
	{0x45,0x5C},
	{0x46,0x00},
	{0x47,0x00},
	{0x48,0x58},
	{0x4A,0x1E},
	{0x4B,0x1E},
	{0x4C,0x00},
	{0x4D,0x00},
	{0x4E,0xA0},
	{0x4F,0x80},
	{0x50,0x00},
	{0x51,0x00},
	{0x52,0x00},
	{0x53,0x00},
	{0x54,0x00},
	{0x57,0x80},
	{0x59,0x10},
	{0x5A,0x08},
	{0x5B,0x94},
	{0x5C,0xE8},
	{0x5D,0x08},
	{0x5E,0x3D},
	{0x5F,0x99},
	{0x60,0x45},
	{0x61,0x40},
	{0x63,0x2D},
	{0x64,0x02},
	{0x65,0x96},
	{0x66,0x00},
	{0x67,0x97},
	{0x68,0x01},
	{0x69,0xCD},
	{0x6A,0x01},
	{0x6B,0xB0},
	{0x6C,0x04},
	{0x6D,0x2C},
	{0x6E,0x01},
	{0x6F,0x32},
	{0x71,0x00},
	{0x72,0x01},
	{0x73,0x35},
	{0x74,0x00},
	{0x75,0x33},
	{0x76,0x31},
	{0x77,0x01},
	{0x7C,0x84},
	{0x7D,0x03},
	{0x7E,0x01},
};

GroveGesture::GroveGesture(int pinsda, int pinscl)
{
    this->i2c = (I2C_T *)malloc(sizeof(I2C_T));
    suli_i2c_init(i2c, pinsda, pinscl);
    
    delay(5);
    
    _init();
}

bool GroveGesture::_init(void)
{
    //Near_normal_mode_V5_6.15mm_121017 for 940nm
    uint8_t error;
    uint8_t data0 = 0, data1 = 0;
	
    //wakeup the sensor
    if(!pajWakeUp()) {
#if USE_DEBUG
        Serial1.println("err: wakeup failed!");
#endif
        return false;
    }
#if USE_DEBUG
	Serial1.println("wakeuped");
#endif	
	//check the addrs
    error = pajReadData(0, 1, &data0);
#if USE_DEBUG
	Serial1.print("Addr0 =");
    Serial1.print(data0 , HEX);
#endif
    if (error != true) {
#if USE_DEBUG
		Serial1.print("error != true1");
#endif
        return false;
    }
    error = pajReadData(1, 1, &data1);
#if USE_DEBUG
	Serial1.print(",  Addr1 =");
    Serial1.println(data1 , HEX);
#endif
    if (error != true) {
#if USE_DEBUG
		Serial1.print("error != true2");
#endif
        return false;
    }
    if ( (data0 != 0x20 ) || (data1 != 0x76) )
        return false;
    
	//setup the gesture sensor
    for (int i = 0; i < INIT_REG_ARRAY_SIZE; i++) {
        pajWriteCmd(init_register_array[i][0], init_register_array[i][1]);
    }
	
	//reselect the bank0 to read data
	pajSelectBank(BANK0);
	
#if USE_DEBUG
    Serial1.println("Paj7620 initialize register.");
#endif
    return true;
}

bool GroveGesture::read_motion(uint8_t *motion)
{
	uint8_t data = 0, data1 = 0, error;
	
	error = pajReadData(0x43, 1, &data);				// Read Bank_0_Reg_0x43/0x44 for gesture result.
	if (error == true)
	{
		*motion = Gesture_None;
		switch (data) 									// When different gestures be detected, the variable 'data' will be set to different values by pajReadData(0x43, 1, &data).
		{
			case GES_RIGHT_FLAG:
				/*delay(GES_ENTRY_TIME);
				pajReadData(0x43, 1, &data);
				if(data == GES_FORWARD_FLAG) 
				{
					*motion = Gesture_Foward;
					delay(GES_QUIT_TIME);
				}
				else if(data == GES_BACKWARD_FLAG) 
				{
					*motion = Gesture_Backward;
					delay(GES_QUIT_TIME);
				}
				else*/
				{
					*motion = Gesture_Right;
				}          
				break;
			case GES_LEFT_FLAG: 
				/*delay(GES_ENTRY_TIME);
				pajReadData(0x43, 1, &data);
				if(data == GES_FORWARD_FLAG) 
				{
					*motion = Gesture_Foward;
					delay(GES_QUIT_TIME);
				}
				else if(data == GES_BACKWARD_FLAG) 
				{
					*motion = Gesture_Backward;
					delay(GES_QUIT_TIME);
				}
				else*/
				{
					*motion = Gesture_Left;
				}          
				break;
			case GES_UP_FLAG:
				/*delay(GES_ENTRY_TIME);
				pajReadData(0x43, 1, &data);
				if(data == GES_FORWARD_FLAG) 
				{
					*motion = Gesture_Foward;
					delay(GES_QUIT_TIME);
				}
				else if(data == GES_BACKWARD_FLAG) 
				{
					*motion = Gesture_Backward;
					delay(GES_QUIT_TIME);
				}
				else*/
				{
					*motion = Gesture_Up;
				}          
				break;
			case GES_DOWN_FLAG:
				/*delay(GES_ENTRY_TIME);
				pajReadData(0x43, 1, &data);
				if(data == GES_FORWARD_FLAG) 
				{
					*motion = Gesture_Foward;
					delay(GES_QUIT_TIME);
				}
				else if(data == GES_BACKWARD_FLAG) 
				{
					*motion = Gesture_Backward;
					delay(GES_QUIT_TIME);
				}
				else*/
				{
					*motion = Gesture_Down;
				}          
				break;
			case GES_FORWARD_FLAG:
				*motion = Gesture_Foward;
				delay(GES_QUIT_TIME);
				break;
			case GES_BACKWARD_FLAG:		  
				*motion = Gesture_Backward;
				delay(GES_QUIT_TIME);
				break;
			case GES_CLOCKWISE_FLAG:
				*motion = Gesture_Clockwise;
				break;
			case GES_COUNT_CLOCKWISE_FLAG:
				*motion = Gesture_CountClockwize;
				break;  
			default:
				pajReadData(0x44, 1, &data1);
				if (data1 == GES_WAVE_FLAG) 
				{
					*motion = Gesture_Wave;
				}
				break;
		}
	}
    return true;
}

/********************************************************
Description:PAJ7620 wake up command
Input:none
Return: uint8_t
********************************************************/
bool GroveGesture::pajWakeUp() {
    uint8_t data;
    uint8_t rtn = suli_i2c_read(this->i2c, PAJ7620_ID, &data, 1);
    if(rtn == 1 && data != 0x20)
        return true;
    else
        return false;
}
/********************************************************
Description:PAJ7620 select register bank
Input:BANK0, BANK1
Return: none
********************************************************/
void GroveGesture::pajSelectBank(uint8_t bank) {
  pajWriteCmd(0xEF, bank);
}

/********************************************************
Description:PAJ7620 Write reg cmd
Input:addr:reg address; cmd:function data
Return: error code; no error return 0
********************************************************/
bool GroveGesture::pajWriteCmd(uint8_t addr, uint8_t cmd) {
    uint8_t rtn = suli_i2c_write_reg(this->i2c, PAJ7620_ID, addr, &cmd, 1);
    if(rtn == 1)
        return true;
    else
        return false;
}

/********************************************************
Description:PAJ7620 read reg data
Input:addr:reg address;
      qty:number of data to read, addr continuously increase;
      data[]:storage memory start address
Return:error code; no error return 0
********************************************************/
bool GroveGesture::pajReadData(uint8_t addr, uint8_t qty, uint8_t data[]) {
    uint8_t rtn = suli_i2c_read_reg(this->i2c, PAJ7620_ID, addr, data, qty);
    if(rtn == qty)
        return true;
    else
        return false;
}



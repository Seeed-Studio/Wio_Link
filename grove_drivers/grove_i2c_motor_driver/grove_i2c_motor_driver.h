/*
 * grove_i2c_motor_driver.h
 *
 * Copyright (c) 2012 seeed technology inc.
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
 */


#ifndef __GROVE_I2C_MOTOR_DRIVER_H__
#define __GROVE_I2C_MOTOR_DRIVER_H__

#include "suli2.h"

//GROVE_NAME        "Grove-I2C Motor Driver"
//SKU               105020001
//IF_TYPE           I2C
//IMAGE_URL         http://www.seeedstudio.com/depot/bmz_cache/a/af1f380a7d22dd28ef29d1af21475b38.image.164x123.jpg

#define MotorSpeedSet             0x82
#define PWMFrequenceSet           0x84
#define DirectionSet              0xaa
#define MotorSetA                 0xa1
#define MotorSetB                 0xa5
#define Nothing                   0x01
#define EnableStepper             0x1a
#define UnenableStepper           0x1b
#define Stepernu                  0x1c

#define I2CMotorDriverAdd         (0x0f<<1)   // Set the address of the I2CMotorDriver

class GroveI2CMotorDriver
{
public:
    GroveI2CMotorDriver(int pinsda, int pinscl);
    
    /**
     * Change ths default I2C address, the default address is 0xf
     * 
     * @param addr_7bits the new 7bits i2c address
     * 
     * @return bool 
     */
    bool write_i2c_address(uint8_t addr_7bits);
    
    /**
     * To drive a stepper motor, we need to change the working mode of this driver into stepper mode. 
     * IMPORTANT: Need I2C Motor Driver hardware version >= v1.3 and manually update the firmware according to the 
     * guide inside this zip file: http://www.seeedstudio.com/wiki/images/5/52/On-Chipfirmware_for_Motor_driver.zip 
     * 
     * @param direction - stepper direction, 0 or 1
     * @param speed - defines the time interval the i2C motor driver change its output to drive the stepper, the actul interval time is : motorspeed * 4ms. that is , when motor speed is 10, the interval time would be 40 ms
     * 
     * @return bool 
     */
    bool write_enable_stepper_mode(uint8_t direction, uint8_t speed);
    
    /**
     * Disable the stepper driving mode, back to default DC motor driving mode
     * 
     * @return bool 
     */
    bool write_disable_stepper_mode();
    
    /**
     * Drive the stepper to move a few steps. To let the stepper rotate forever, steps = 255. 
     * IMPORTANT: Need I2C Motor Driver hardware version >= v1.3 and manually update the firmware according to the 
     * guide inside this zip file: http://www.seeedstudio.com/wiki/images/5/52/On-Chipfirmware_for_Motor_driver.zip  
     * 
     * @param steps - number of steps to move
     * 
     * @return bool 
     */
    bool write_stepper_steps(uint8_t steps);
    
    /**
     * Change the speed of DC motor, note that the initial speed is 0 when the module is powered on.
     * 
     * @param speed_m1 - 0~255
     * @param speed_m2 - 0~255
     * 
     * @return bool 
     */
    bool write_dcmotor_speed(uint8_t speed_m1, uint8_t speed_m2);
    
    bool write_dcmotor1_change_direction();
    bool write_dcmotor2_change_direction();
    bool write_dcmotor1_break();
    bool write_dcmotor2_break();
    bool write_dcmotor1_resume();
    bool write_dcmotor2_resume();
    
private:
    I2C_T *i2c;
    uint8_t i2c_addr;
    uint8_t dir;
    uint8_t cmd[3];
    bool motor1_dir, motor2_dir;
};


#endif

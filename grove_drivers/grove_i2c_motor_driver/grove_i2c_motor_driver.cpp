/*
 * grove_i2c_motor_driver.cpp
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

#include "suli2.h"
#include "grove_i2c_motor_driver.h"

GroveI2CMotorDriver::GroveI2CMotorDriver(int pinsda, int pinscl)
{
    this->i2c = (I2C_T *)malloc(sizeof(I2C_T));
    suli_i2c_init(this->i2c, pinsda, pinscl);
    
    dir = 0xa;
    i2c_addr = I2CMotorDriverAdd;
    motor1_dir = 0x2;
    motor2_dir = 0x2;
}

bool GroveI2CMotorDriver::write_i2c_address(uint8_t addr_7bits)
{
    i2c_addr = addr_7bits << 1;
    return true;
}

bool GroveI2CMotorDriver::write_enable_stepper_mode(uint8_t direction, uint8_t speed)
{
    cmd[0] = EnableStepper;
    cmd[1] = direction;
    cmd[2] = speed;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}

bool GroveI2CMotorDriver::write_disable_stepper_mode()
{
    cmd[0] = UnenableStepper;
    cmd[1] = Nothing;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}

bool GroveI2CMotorDriver::write_stepper_steps(uint8_t steps)
{
    cmd[0] = Stepernu;
    cmd[1] = steps;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}

bool GroveI2CMotorDriver::write_dcmotor_speed(uint8_t speed_m1, uint8_t speed_m2)
{
    cmd[0] = MotorSpeedSet;
    cmd[1] = speed_m1;
    cmd[2] = speed_m2;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    
    cmd[0] = DirectionSet;
    cmd[1] = dir;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    
    return true;
}

bool GroveI2CMotorDriver::write_dcmotor1_change_direction()
{
    dir = dir ^ 0x3;
    
    cmd[0] = DirectionSet;
    cmd[1] = dir;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}

bool GroveI2CMotorDriver::write_dcmotor2_change_direction()
{
    dir = dir ^ 0xC;
    
    cmd[0] = DirectionSet;
    cmd[1] = dir;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}

bool GroveI2CMotorDriver::write_dcmotor1_break()
{   
    motor1_dir = dir & 0x3;
    dir |= 0x3;
    
    cmd[0] = DirectionSet;
    cmd[1] = dir;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}

bool GroveI2CMotorDriver::write_dcmotor2_break()
{
    motor2_dir = (dir>>2) & 0x3;
    dir |= 0xc;
    
    cmd[0] = DirectionSet;
    cmd[1] = dir;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}

bool GroveI2CMotorDriver::write_dcmotor1_resume()
{
    dir = (dir & 0xc) | motor1_dir;
    cmd[0] = DirectionSet;
    cmd[1] = dir;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}

bool GroveI2CMotorDriver::write_dcmotor2_resume()
{
    dir = (dir & 0x3) | (motor2_dir<<2);
    
    cmd[0] = DirectionSet;
    cmd[1] = dir;
    cmd[2] = Nothing;
    suli_i2c_write(i2c, i2c_addr, cmd, 3);
    return true;
}



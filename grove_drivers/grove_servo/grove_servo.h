/*
 * grove_servo.h
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


#ifndef __GROVE_SERVO_H__
#define __GROVE_SERVO_H__

#include "suli2.h"

//GROVE_NAME        "Grove-Servo"
//SKU               316010005
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/thumb/0/0f/Grove%E2%80%94Servo.jpg/600px-Grove%E2%80%94Servo.jpg


class GroveServo
{
public:
    GroveServo(int pin);
    
    /**
     * Drive the servo to rotate a specified angle and hold on the servo driven PWM signal. <br>
     * The PWM signal maybe polluted by other PWM API calls if the others use different frequency. <br> 
     * e.g. Grove - Infrared Emitter may affect the action of servo as it's using 38KHz PWM.
     * 
     * @param degree - the angle in unit degress
     * 
     * @return bool 
     */
    bool write_angle(int degree);
    
    /**
     * Drive the servo to rotate a specified angle and shut down the servo driven PWM signal in specified seconds time. <br> 
     * For normal servos, this API still works even if the PWM signal is off. But the servo losts the strength that holding the position. <br> 
     * This API will avoid the servo from the influence of other PWM modules, e.g. Grove - Infrared Emitter
     * 
     * @param degree - the angle in unit degress
     * @param seconds - the duration of the motion
     * 
     * @return bool 
     */
    bool write_angle_motion_in_seconds(int degree, int seconds);



    PWM_T *io;
    TIMER_T *timer;
    
};

static void grove_servo_timer_interrupt_handler(void *para);



#endif

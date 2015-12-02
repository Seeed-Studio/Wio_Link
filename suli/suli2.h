/*
  suli2.h
  Seeed Unified Library Interface v2.0
  2015 Copyright (c) Seeed Technology Inc.  All right reserved.

  Author:Jack Shao, Loovee
  Change Logs:
  2015-4-17: initial version for v2

  suli is designed for the purpose of reusing the high level implementation
  of each libraries for different platforms out of the hardware layer.
  suli2 is the new reversion of original suli. There're lot of improvements upon
  the previous version. Currently, it can be treated as the internal strategy for
  quick library development of seeed. But always welcome the community to
  follow the basis of suli to contribute grove libraries.

  The MIT License (MIT)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef __SULI2_H__
#define __SULI2_H__

#if defined(__MBED__)
#include "mbed.h"
#elif defined(ARDUINO)
#include "Arduino.h"
#endif


/***************************************************************************
 * IO related APIs
 ***************************************************************************/
//-------------- mbed ---------------
#if defined(__MBED__)

typedef gpio_t IO_T;

#define SULI_INPUT    PIN_INPUT
#define SULI_OUTPUT   PIN_OUTPUT
#define SULI_HIGH     0x01
#define SULI_LOW      0x00
#define SULI_RISE     IRQ_RISE
#define SULI_FALL     IRQ_FALL  
#define SULI_CHANGE   IRQ_RISE   //mbed doesn't support change interrupt.
/**
 * void suli_pin_init(IO_T *, PIN_T, PIN_DIR )
 */
#define suli_pin_init(pio,pin,dir) {gpio_set((PinName)pin); gpio_init(pio, (PinName)pin); gpio_dir(pio, dir);}

/**
 * void suli_pin_dir(IO_T *, PIN_DIR )
 */
#define suli_pin_dir(pio,dir)  gpio_dir(pio, dir)

/**
 * void suli_pin_write(IO_T *, PIN_HIGH_LOW)
 */
#define suli_pin_write(pio,state)  gpio_write(pio, state)

/**
 * int suli_pin_read(IO_T *)
 */
#define suli_pin_read(pio) gpio_read(pio)

/**
 * uint32_t suli_pin_pulse_in(IO_T *, what_state, timeout)
 */
uint32_t suli_pin_pulse_in(IO_T *pio, int state, uint32_t timeout);

#define  suli_pin_attach_interrupt_handler(pio, handler, mode, para)


//-------------- arduino ---------------
#elif defined(ARDUINO)

typedef int IO_T;

#define SULI_INPUT    INPUT
#define SULI_OUTPUT   OUTPUT
#define SULI_HIGH     0x01
#define SULI_LOW      0x00
#define SULI_RISE     RISING
#define SULI_FALL     FALLING  
#define SULI_CHANGE   CHANGE

/**
 * void suli_pin_init(IO_T *, PIN_T, PIN_DIR )
 */
#define suli_pin_init(pio, pin, dir) {*(pio) = pin; pinMode(pin, dir);}

/**
 * void suli_pin_dir(IO_T *, PIN_DIR )
 */
#define suli_pin_dir(pio,dir)  pinMode(*(pio), dir)

/**
 * void suli_pin_write(IO_T *, PIN_HIGH_LOW)
 */
#define suli_pin_write(pio,state)  digitalWrite(*(pio), state)

/**
 * int suli_pin_read(IO_T *)
 */
#define suli_pin_read(pio)  digitalRead(*(pio))

/**
 * uint32_t suli_pin_pulse_in(IO_T *, what_state, timeout) 
 * @param timeout - us 
 */
#define suli_pin_pulse_in(pio,state,timeout)  pulseIn(*(pio), state, timeout)

/**
 * attach a handler to a interrupt with a parameter ( e.g. a
 * class instance poiter )
 */
#ifdef ESP8266_SEEED_NODE
typedef void (*interrupt_handler)(void *para);
extern "C"
void attachInterruptEx(uint8_t pin, interrupt_handler userFunc, int mode, void *para);

void suli_pin_attach_interrupt_handler(IO_T *pio, interrupt_handler handler, int mode, void *para);
#else
#define  suli_pin_attach_interrupt_handler(pio, handler, mode, para)
#endif

#endif


/***************************************************************************
 * Analog related APIs
 ***************************************************************************/
//-------------- mbed ---------------
#if defined(__MBED__)

typedef analogin_t ANALOG_T;

/**
 * NOTE:
 * To make the higher level driver compatible with each other
 * among several platforms, we unify the resolution of ADC to
 * 10bits. We think 10bits is enough for analog reading. So if
 * your ADC is 12bit, you need to >>2, or your ADC is 8Bit, you
 * need to <<2
 */

/**
 * void suli_analog_init(ANALOG_T *aio, int pin);
 */
#define suli_analog_init(aio, pin)    analogin_init(aio, (PinName)pin)

/**
 * uint16_t suli_analog_read(ANALOG_T *)
 */
#define suli_analog_read(aio)    ((uint16_t)(analogin_read_u16(aio)>>6))


//-------------- Arduino ---------------
#elif defined(ARDUINO)

typedef int ANALOG_T;
/**
 * void suli_analog_init(ANALOG_T *aio, int pin);
 */
#define suli_analog_init(aio, pin)    {*aio = pin; pinMode(pin, INPUT);}

/**
 * uint16_t suli_analog_read(ANALOG_T *)
 */
#define suli_analog_read(aio)    analogRead(*aio)


#endif


/***************************************************************************
 * PWM related APIs
 ***************************************************************************/
//-------------- mbed ---------------
#if defined(__MBED__)

typedef pwmout_t PWM_T;

/**
 * void suli_pwm_init(PWM_T *, int pin)
 */
inline void suli_pwm_init(PWM_T *pwm, int pin)
{
    pwmout_init(pwm, (PinName)pin);
}

/**
 * void suli_pwm_frequency(PWM_T *, uint32_t Hz)
 */
inline void suli_pwm_frequency(PWM_T *pwm, uint32_t hz)
{
    int us = 1000000 / hz;
    pwmout_period_us(pwm, us);
}

/**
 * void suli_pwm_output(PWM_T *, float percent)
 */
inline void suli_pwm_output(PWM_T *pwm, float percent)
{
    pwmout_write(pwm, percent);
}


//-------------- Arduino ---------------
#elif defined(ARDUINO)

typedef int PWM_T;

/**
 * void suli_pwm_init(PWM_T *, int pin)
 */
inline void suli_pwm_init(PWM_T *pwm, int pin)
{
    *pwm = pin;
    pinMode(pin, OUTPUT);
}

/**
 * void suli_pwm_frequency(PWM_T *, uint32_t Hz)
 * This method is not implemented for Arduino because changing
 * the PWM frequency will also change the timer related API.
 * So the default frequencies are:
 * Arduino Pins 5 and 6: 1kHz
 * Arduino Pins 9, 10, 11, and 3: 500Hz
 */
inline void suli_pwm_frequency(PWM_T *pwm, uint32_t hz)
{
#ifdef ESP8266
    analogWriteFreq(hz);
#endif
}

/**
 * void suli_pwm_output(PWM_T *, float percent) 
 * percent: 0.0 ~ 1.0f 
 */
inline void suli_pwm_output(PWM_T *pwm, float percent)
{
#ifdef ESP8266
    int duty = constrain((int)(1023 * percent), 0, 1023);
#else
    uint8_t duty = constrain((int)(255.0 * percent), 0, 255);
#endif
    analogWrite(*pwm, duty);
}


#endif


/***************************************************************************
 * Time related APIs
 ***************************************************************************/
//-------------- mbed ---------------
#if defined(__MBED__)

/*
 * delay
 */
#define suli_delay_us(us)   wait_us(us)
#define suli_delay_ms(ms)   wait_ms(ms)


/*
 * Returns the number of milliseconds since your board began running the current program.
 * This number will overflow (go back to zero), after approximately 50 days.
*/
#define suli_millis()   (us_ticker_read()/1000)


/*
 * Returns the number of microseconds since your board began running the current program.
 * This number will overflow (go back to zero), after approximately 70 minutes.
 * Note: there are 1,000 microseconds in a millisecond and 1,000,000 microseconds in a second.
 */
#define suli_micros()   us_ticker_read()


//-------------- Arduino ---------------
#elif defined(ARDUINO)

#define suli_delay_us(us)  delayMicroseconds(us)
#define suli_delay_ms(ms)  delay(ms)
#define suli_millis()      millis()
#define suli_micros()      micros()

#endif


/***************************************************************************
 * I2C related APIs
 ***************************************************************************/
//-------------- mbed ---------------
#if defined(__MBED__)

typedef i2c_t I2C_T;

/**
 * I2C interface initialize.
 */
inline void suli_i2c_init(I2C_T *i2c_device, int pin_sda, int pin_clk)
{
    i2c_init(i2c_device, (PinName)pin_sda, (PinName)pin_clk);
    i2c_frequency(i2c_device, 100000);
}

/**
 * write a buff to I2C
 * dev_addr: 8bits address
 */
inline uint8_t suli_i2c_write(I2C_T *i2c_device, uint8_t dev_addr, uint8_t *data, int len)
{
    return i2c_write(i2c_device, (int)dev_addr, (const char *)data, len, 1);
}

/**
 * write a buff to reg adress started from reg_addr
 * I2C dev_addr: 8bits address
 */
uint8_t suli_i2c_write_reg(I2C_T *i2c_device, uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, int len);

/**
 * read data from I2C
 * dev_addr: 8bits address
 */
inline uint8_t suli_i2c_read(I2C_T *i2c_device, uint8_t dev_addr, uint8_t *buff, int len)
{
    return i2c_read(i2c_device, (int)dev_addr, (char *)buff, len, 1);
}

/**
 * read data from I2C's reg_adress
 * dev_addr: 8bits address
 */
uint8_t suli_i2c_read(I2C_T *i2c_device, uint8_t dev_addr, uint8_t reg_addr, uint8_t *buff, int len);




//-------------- Arduino ---------------
#elif defined(ARDUINO) && (defined (ARDUINO_USE_I2C) || defined(ESP8266))

#include <Wire.h>

typedef TwoWire *I2C_T;
/**
 * I2C interface initialize.
 * ignore pin definations for Arduino
 */
void suli_i2c_init(I2C_T *i2c_device, int pin_sda=0, int pin_clk=0);


/**
 * write a buff to I2C
 * dev_addr: 8bits address
 */
uint8_t suli_i2c_write(I2C_T *i2c_device, uint8_t dev_addr, uint8_t *data, int len);

/**
 * write a buff to reg adress started from reg_addr
 * I2C dev_addr: 8bits address
 */
uint8_t suli_i2c_write_reg(I2C_T *i2c_device, uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, int len);

/**
 * read data from I2C
 * dev_addr: 8bits address
 */
uint8_t suli_i2c_read(I2C_T *i2c_device, uint8_t dev_addr, uint8_t *buff, int len);

/**
 * read data from I2C's reg_adress
 * dev_addr: 8bits address
 */
uint8_t suli_i2c_read_reg(I2C_T *i2c_device, uint8_t dev_addr, uint8_t reg_addr, uint8_t *buff, int len);

#endif


/***************************************************************************
 * UART related APIs
 ***************************************************************************/
//-------------- mbed ---------------
#if defined(__MBED__)

typedef serial_t UART_T;
typedef void (*cb_fun_ptr)(void);//jacly add

/**
 * void suli_uart_init(UART_T *, int pin_tx, int pin_rx, uint32_t baud)
 */
inline void suli_uart_init(UART_T *uart, int pin_tx, int pin_rx, uint32_t baud)
{
    serial_init(uart, (PinName)pin_tx, (PinName)pin_rx);
    serial_baud(uart, (int)baud);
}

/**
jacly add
 * void suli_uart_attach(UART_T *uart, SerialIrq irq, uint32_t enable)
 */
inline void suli_uart_rx_event_attach(UART_T *uart, cb_fun_ptr irq)
{
    serial_irq_handler(uart, (uart_irq_handler)irq, 1);
    serial_irq_set(uart, RxIrq, 1);
}

/**
 * send a byte to uart
 * void suli_uart_write(UART_T *, uint8_t)
 */
inline void suli_uart_write(UART_T *uart, uint8_t data)
{
    serial_putc(uart, (int)data);
}

/**
 * send bytes to uart
 * int suli_uart_write_bytes(UART_T *, uint8_t *, int)
 */
int suli_uart_write_bytes(UART_T *uart, uint8_t *data, int len);

/**
 * write a float
 * num - number to write
 * decimal - x decimal point
 */
void suli_uart_write_float(UART_T *uart, float float_num, int decimal=2);

/**
 * write an integer
 * num - number to write
 */
void suli_uart_write_int(UART_T *uart, int32_t num);


/**
 * read a byte from uart
 */
inline uint8_t suli_uart_read(UART_T *uart)
{
    return (uint8_t)serial_getc(uart);
}

/**
 * read bytes from uart
 */
int suli_uart_read_bytes(UART_T *uart, uint8_t *buff, int len);

/**
 * read bytes from uart with timeout ms
 */
int suli_uart_read_bytes_timeout(UART_T *uart, uint8_t *buff, int len, int timeout_ms=1000);

/**
 * if uart get data, return 1-readable, 0-unreadable
 */
inline int suli_uart_readable(UART_T *uart)
{
    return serial_readable(uart);
}





//-------------- Arduino ---------------
#elif defined(ARDUINO)

typedef Stream* UART_T;

/**
 * void suli_uart_init(UART_T *, int pin_tx, int pin_rx, uint32_t baud)
 * if you want to use softwareSerial, you need to include
 * "SoftwareSerial.h" in sketchbook, and #define ARDUINO_SOFTWARE_SERIAL
 */
void suli_uart_init(UART_T *uart, int pin_tx, int pin_rx, uint32_t baud);

/**
 * send a byte to uart
 * void suli_uart_write(UART_T *, uint8_t)
 */
inline void suli_uart_write(UART_T *uart, uint8_t data)
{
    (*uart)->write(data);
}

/**
 * send bytes to uart
 * int suli_uart_write_bytes(UART_T *, uint8_t *, int)
 */
int suli_uart_write_bytes(UART_T *uart, uint8_t *data, int len);

/**
 * write a float
 * num - number to write
 * decimal - x decimal point
 */
inline void suli_uart_write_float(UART_T *uart, float float_num, int decimal = 2)
{
    (*uart)->print(float_num, decimal);
}
/**
 * write an integer
 * num - number to write
 */
inline void suli_uart_write_int(UART_T *uart, int32_t num)
{
    (*uart)->print(num);
}

/**
 * read a byte from uart
 */
inline uint8_t suli_uart_read(UART_T *uart)
{
    return (uint8_t)(*uart)->read();
}

/**
 * read bytes from uart
 */
inline int suli_uart_read_bytes(UART_T *uart, uint8_t *buff, int len)
{
    return (*uart)->readBytes((char *)buff, len);
}

/**
 * read bytes from uart with timeout ms
 */
inline int suli_uart_read_bytes_timeout(UART_T *uart, uint8_t *buff, int len, int timeout_ms = 1000)
{
    (*uart)->setTimeout((unsigned long)timeout_ms);
    return (*uart)->readBytes((char *)buff, len);
}

/**
 * if uart get data, return 1-readable, 0-unreadable
 */
inline int suli_uart_readable(UART_T *uart)
{
    return (*uart)->available();
}

#endif


/***************************************************************************
 * Event related APIs
 ***************************************************************************/
typedef void (*event_callback_t)(char *event_name, uint32_t event_data);
struct __event_s
{
    event_callback_t    cb;
    char                *event_name;
};
typedef __event_s               EVENT_T;
typedef event_callback_t        CALLBACK_T;

inline void suli_event_init(EVENT_T *event, CALLBACK_T cb, char *name)
{
    event->cb = cb;
    event->event_name = name;
}
inline void suli_event_trigger(EVENT_T *event, uint32_t event_data)
{
    if (event->cb)
    {
        (event->cb)(event->event_name, event_data);
    }
}


#endif  //__SULI2_H__

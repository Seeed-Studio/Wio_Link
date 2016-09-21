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

/***************************************************************************
 * This suli2 is for the ESP8266 platform.
 ***************************************************************************/

#include "Arduino.h"


/***************************************************************************
 * Common
 ***************************************************************************/
#ifndef constrain(amt,low,high)
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif

long suli_map(long x, long in_min, long in_max, long out_min, long out_max);

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR
#endif


/***************************************************************************
 * IO related APIs
 ***************************************************************************/

typedef int IO_T;

#define SULI_INPUT    INPUT     // the default high resistance input
#define SULI_INPUT_PULLUP       INPUT_PULLUP
#define SULI_INPUT_PULLDOWN     INPUT_PULLDOWN
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
typedef void (*interrupt_handler)(void *para);
extern "C"
void attachInterruptEx(uint8_t pin, interrupt_handler userFunc, int mode, void *para);

/**
 * with the arduino implementation, we can attach only 1 handler to 1 pin.
 */
void suli_pin_attach_interrupt_handler(IO_T *pio, interrupt_handler handler, int mode, void *para);


/***************************************************************************
 * Analog related APIs
 ***************************************************************************/

typedef int ANALOG_T;

#define SULI_ANALOG_MAX_READING             1023
#define SULI_ANALOG_MAX_VOLTAGE             3.0f  //3.0V, this is due to the voltage division 1/3, not 1/3.3, ESP8266's analog full range is 1V.
#define SULI_PLATFORM_VCC                   3.3f

/**
 * void suli_analog_init(ANALOG_T *aio, int pin);
 */
#define suli_analog_init(aio, pin)    {*aio = pin; pinMode(pin, INPUT);}

/**
 * uint16_t suli_analog_read(ANALOG_T *)
 */
//fix resistor division error
#define suli_analog_read(aio)    ((int)analogRead(*aio))

/**
 * get the voltage (V) of measuring point, indepencent of the reference voltage
 * float suli_analog_voltage(ANALOG_T *)
 * @return float voltage, unit V
 */
inline float suli_analog_voltage(ANALOG_T *aio)
{
    return SULI_ANALOG_MAX_VOLTAGE * suli_analog_read(aio) / SULI_ANALOG_MAX_READING;
}

/***************************************************************************
 * PWM related APIs
 ***************************************************************************/

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
    analogWriteFreq(hz);
}

/**
 * void suli_pwm_output(PWM_T *, float percent)
 * percent: 0.0 ~ 100.0f
 */
inline void suli_pwm_output(PWM_T *pwm, float percent)
{
#ifdef ESP8266
    int duty = constrain((int)(10.23 * percent), 0, 1023);
#else
    uint8_t duty = constrain((int)(2.55 * percent), 0, 255);
#endif
    analogWrite(*pwm, duty);
}

/***************************************************************************
 * Time related APIs
 ***************************************************************************/

#define suli_delay_us(us)  delayMicroseconds(us)
#define suli_delay_ms(ms)  delay(ms)
#define suli_millis()      millis()
#define suli_micros()      micros()


/***************************************************************************
 * I2C related APIs
 ***************************************************************************/

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


/***************************************************************************
 * UART related APIs
 ***************************************************************************/

typedef HardwareSerial *UART_T;

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
 * print string on uart
 * int suli_uart_print(UART_T *, uint8_t *, int)
 */
inline int suli_uart_print(UART_T *uart, char *str)
{
    (*uart)->print((const char *)str);
}

/**
 * print string and new line on uart
 * int suli_uart_println(UART_T *, uint8_t *, int)
 */
inline int suli_uart_println(UART_T *uart, char *str)
{
    (*uart)->println((const char *)str);
}

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

/**
 * The debug serial should be initialize by the firmware framework scope,
 * and then be refered by the drivers. The firmware framework may control
 * the enable state of debug serial.
 *
 * @param uart
 */
void suli_set_debug_serial(UART_T *uart);

/**
 * Get the pointer of the debug serial.
 *
 * @return UART_T*
 */
UART_T *suli_get_debug_serial();

/***************************************************************************
 * Event related APIs
 ***************************************************************************/
typedef enum
{
    SULI_EDT_NONE, SULI_EDT_BOOL, SULI_EDT_UINT8, SULI_EDT_INT8, SULI_EDT_UINT16, SULI_EDT_INT16, SULI_EDT_INT,
    SULI_EDT_UINT32, SULI_EDT_INT32, SULI_EDT_FLOAT, SULI_EDT_STRING
}EVENT_DATA_TYPE_T;

typedef void (*event_callback_t)(char *event_name, void *event_data, int event_data_type);
#define EVENT_CALLBACK_T        event_callback_t

struct __event_s
{
    event_callback_t    cb;
    char                *event_name;
    int                 event_data_type;
};
typedef __event_s    EVENT_T;

void suli_event_init(EVENT_T *event, EVENT_CALLBACK_T cb, char *name, EVENT_DATA_TYPE_T event_data_type);

void suli_event_trigger(EVENT_T *event, void *event_data);


#define DEFINE_EVENT(EVENT_NAME, EVENT_DATA_TYPE)    \
    public: \
    EVENT_T event_##EVENT_NAME ; \
    inline EVENT_T *attach_event_reporter_for_##EVENT_NAME (EVENT_CALLBACK_T reporter) \
    { \
        suli_event_init(&event_##EVENT_NAME , reporter, #EVENT_NAME , EVENT_DATA_TYPE); \
        return &event_##EVENT_NAME ; \
    }

#define POST_EVENT(EVENT_NAME, P_EVENT_DATA)    suli_event_trigger(&event_##EVENT_NAME , (void *)(P_EVENT_DATA))

#define POST_EVENT_IN_INSTANCE(INSTANCE, EVENT_NAME, P_EVENT_DATA)   suli_event_trigger(&INSTANCE->event_##EVENT_NAME, (void *)(P_EVENT_DATA))

#define ATTACH_CALLBACK_FOR_EVENT(EVENT_NAME, CALLBACK)    attach_event_reporter_for_##EVENT_NAME (CALLBACK)



/***************************************************************************
 * Timer related APIs
 ***************************************************************************/
typedef void (*timer_callback_t)(void *data);

typedef struct __timer_s
{
    timer_callback_t    cb;
    uint32_t            interval_ticks;
    uint32_t            fire_ticks;
    struct __timer_s    *prev;
    struct __timer_s    *next;
    void                *data;
    bool                repeat;
}TIMER_T;

/* for esp8266, use timer0, it is not the one PWM used (timer1) */
void __suli_timer_isr();

void __suli_timer_hw_init();
void __suli_timer_enable_interrupt();
void __suli_timer_disable_interrupt();

inline uint32_t __suli_timer_get_ticks()
{
    uint32_t ccount;
    __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
    return ccount;

}

#define __suli_timer_microseconds_to_ticks     microsecondsToClockCycles

inline void __suli_timer_set_timeout_ticks(uint32_t ticks)
{
    timer0_write(__suli_timer_get_ticks() + ticks);
}


/** platform indepencent funtions*/

/**-----------------------------------------
 * Hardware timer APIs
 * The hardware timer APIs implement a timer event loop based on hardware timers,
 * it's realtime so that it MUST NOT use the CPU too long.
 -----------------------------------------*/

/**
 * Install a timer with interval microseconds
 *
 * @param timer - the pointer to pre-allocate TIMER_T struct
 * @param microseconds - interval length (for esp8266's 32bit timer0, the longest interval is ~52000000 us.)
 * @param cb - callback function for this timer entry
 * @param repeat - if repeat loading when fired
 */
void suli_timer_install(TIMER_T *timer, uint32_t microseconds, timer_callback_t cb, void *data, bool repeat = false);

/**
 * Remove the timer.
 * Note the function will not release the memory
 *
 * @param timer
 */
void suli_timer_remove(TIMER_T *timer);

/**
 * Change the interval of this timer.
 *
 * @param timer
 * @param microseconds
 */
void suli_timer_control_interval(TIMER_T *timer, uint32_t microseconds);

/**-----------------------------------------
 * Software timer APIs
 * The software timer is for those periodly occurred events/tasks which
 * don't require realtime. The soft timer inserts fire-points inside the main loop,
 * For non-RTOS platforms, the main loop does lots of things, and maybe delayed
 * by suli_delay_xs(), so soft timer is not accurate at all.
 * e.g. We install a soft timer whose fire time is 1s later, and next time the main
 * loop reaches soft timer check function, it's already passed 2s, so the soft timer
 * will fire then, but it's fired later than the time it desired.
 -----------------------------------------*/

/**
 * Install a soft timer in the main loop
 * Note that the time unit is millisecond
 *
 * @param timer
 * @param milliseconds
 * @param cb
 * @param data
 * @param repeat
 */
void suli_soft_timer_install(TIMER_T *timer, uint32_t milliseconds, timer_callback_t cb, void *data, bool repeat = false);

void suli_soft_timer_remove(TIMER_T *timer);

void suli_soft_timer_control_interval(TIMER_T *timer, uint32_t milliseconds);

void suli_soft_timer_loop();


#endif  //__SULI2_H__

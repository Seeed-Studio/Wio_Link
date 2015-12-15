/*
 * grove_ir_emitter.cpp
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
#include "grove_ir_emitter.h"


GroveIREmit::GroveIREmit(int pin)
{
    this->io = (PWM_T *)malloc(sizeof(PWM_T));
    this->data = (uint8_t *)malloc(26);

    _khz = 38;
    
    suli_pwm_init(this->io, pin);
    suli_pwm_output(this->io, 0);
    suli_pin_write(this->io, SULI_LOW);
    
    
    data[D_STARTH] = 168;
    data[D_STARTL] = 81;
    data[D_SHORT] = 11;
    data[D_LONG] = 29;
}

bool GroveIREmit::write_protocol_parameters(uint8_t start_h, uint8_t start_l, uint8_t n_short, uint8_t n_long)
{
    data[D_STARTH] = start_h;
    data[D_STARTL] = start_l;
    data[D_SHORT] = n_short;
    data[D_LONG] = n_long;
    return true;
    
}

bool GroveIREmit::read_protocal_parametersbool(uint8_t *start_h, uint8_t *start_l, uint8_t *n_short, uint8_t *n_long)
{
    *start_h = data[D_STARTH];
    *start_l = data[D_STARTL];
    *n_short = data[D_SHORT];
    *n_long = data[D_LONG];
    return true;
    
}


bool GroveIREmit::write_data_hex(char *data_hex)
{
    return write_data_hex_in_freq(38, data_hex);
}

bool GroveIREmit::write_data_hex_in_freq(uint16_t freq_khz, char *data_hex)
{
    if (!_extract_data_from_string(data_hex))
    {
        return false;
    }

    Send(data, freq_khz);

    return true;
}

bool GroveIREmit::_extract_data_from_string(char *str)
{
    int len = strlen(str);
    len = len - len % 2;
    int cnt = len / 2;

    cnt = min(cnt, 20);
    
    data[D_DATALEN] = cnt;

    uint8_t hex[3];
    hex[2] = '\0';
    uint32_t ul;

    for (int i = 0; i < cnt; i++)
    {
        memcpy(hex, (str + i * 2), 2);
        ul = strtoul((const char *)hex, NULL, 16);
        if (ul >= 0 && ul <= 255)
        {
            data[6+i] = ul;
        } else
        {
            error_desc = "invalid Hex value";
            return false;
        }
    }

#if __IR_EMITTER_DEBUG
    hexdump(data, 26);
#endif
    return true;
}


void GroveIREmit::enableIROut(int khz)
{
    // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
    // The IR output will be on pin 3 (OC2B).
    // This routine is designed for 36-40KHz; if you use it for other values, it's up to you
    // to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
    // TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
    // controlling the duty cycle.
    // There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
    // To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
    // A few hours staring at the ATmega documentation and this will all make sense.
    // See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.

    // Disable the Timer2 Interrupt (which is used for receiving IR)
    //TIMER_DISABLE_INTR; //Timer2 Overflow Interrupt

    //pinMode(TIMER_PWM_PIN, OUTPUT);
    //digitalWrite(TIMER_PWM_PIN, LOW); // When not sending PWM, we want it low

    //TIMER_CONFIG_KHZ(khz);
    //TIMER_ENABLE_PWM;
    
    _khz = khz;
    
    suli_pwm_output(this->io, 0);
    suli_pin_write(this->io, SULI_LOW);
    suli_pwm_frequency(this->io, khz * 1000);
    suli_pwm_output(this->io, 50.0);

}

void GroveIREmit::sendRaw(unsigned int buf[], int len, int hz)
{
    enableIROut(hz);

    for (int i = 0; i < len; i++)
    {
        if (i & 1)
        {
            space(buf[i]);
        } else
        {
            mark(buf[i]);
        }
    }
    space(0); // Just to be sure
}

void GroveIREmit::mark(int time)
{
    // Sends an IR mark for the specified number of microseconds.
    // The mark output is modulated at the PWM frequency.
    //TIMER_ENABLE_PWM; // Enable pin 3 PWM output
    suli_pwm_output(this->io, 50.0);
    suli_delay_us(time);
}

/* Leave pin off for time (given in microseconds) */
void GroveIREmit::space(int time)
{
    // Sends an IR space for the specified number of microseconds.
    // A space is no output, so the PWM output is disabled.
    //TIMER_DISABLE_PWM; // Disable pin 3 PWM output
    suli_pwm_output(this->io, 0);
    suli_pin_write(this->io, SULI_LOW);
    suli_delay_us(time);
}

void GroveIREmit::Send(unsigned char *idata, unsigned char ifreq)
{
    int len = idata[0];
    unsigned char start_high    = idata[1];
    unsigned char start_low     = idata[2];
    unsigned char nshort        = idata[3];
    unsigned char nlong         = idata[4];
    unsigned char datalen       = idata[5];

    unsigned int *pSt = (unsigned int *)malloc((4 + datalen * 16) * sizeof(unsigned int));

    if (NULL == pSt)
    {
#if __IR_EMITTER_DEBUG
        Serial1.println("not enough place!!\r\n");
#endif
        return;
    }

#if __IR_EMITTER_DEBUG
    Serial1.println("begin to send ir:\r\n");
    Serial1.print("ifreq = "); Serial1.println(ifreq);
    Serial1.print("len = "); Serial1.println(len);
    Serial1.print("start_high = "); Serial1.println(start_high);
    Serial1.print("start_low = "); Serial1.println(start_low);
    Serial1.print("nshort = "); Serial1.println(nshort);
    Serial1.print("nlong = "); Serial1.println(nlong);
    Serial1.print("datalen = "); Serial1.println(datalen);
#endif

    pSt[0] = start_high * 50;
    pSt[1] = start_low * 50;

    for (int i = 0; i < datalen; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (idata[6 + i] & 0x01 << (7 - j))
            {
                pSt[16 * i + 2 * j + 2] = nshort * 50;
                pSt[16 * i + 2 * j + 3] = nlong * 50;
            } else
            {
                pSt[16 * i + 2 * j + 2] = nshort * 50;
                pSt[16 * i + 2 * j + 3] = nshort * 50;
            }
        }
    }

    pSt[2 + datalen * 16] = nshort * 50;
    pSt[2 + datalen * 16 + 1] = nshort * 50;

#if 0
    for (int i = 0; i < 4 + datalen * 16; i++)
    {
        Serial1.print(pSt[i]); Serial1.print("\t");
    }
    Serial1.println();
#endif
    sendRaw(pSt, 4 + datalen * 16, ifreq);
    free(pSt);

}



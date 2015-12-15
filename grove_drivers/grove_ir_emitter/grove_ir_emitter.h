/*
 * grove_ir_emitter.h
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


#ifndef __GROVE_IR_EMITTER_H__
#define __GROVE_IR_EMITTER_H__

#include "suli2.h"

//GROVE_NAME        "Grove - Infrared Emitter"
//SKU               101020026
//IF_TYPE           GPIO
//IMAGE_URL         http://www.seeedstudio.com/wiki/images/thumb/6/6a/Grove_-_Infrared_Emitter.jpg/400px-Grove_-_Infrared_Emitter.jpg


#define __IR_EMITTER_DEBUG    0

#define D_LEN       0
#define D_STARTH    1
#define D_STARTL    2
#define D_SHORT     3
#define D_LONG      4
#define D_DATALEN   5
#define D_DATA      6



class GroveIREmit
{
public:
    GroveIREmit(int pin);
    
    /**
     * Send a sequence of data with the emitter, using frequency 38KHz.
     * 
     * @param data_hex - a string in hex format, e.g. FFBBCC0011
     * 
     * @return bool 
     */
    bool write_data_hex(char *data_hex);
    
    /**
     * Send a sequence of data with the emitter, using a specified frequency.
     * 
     * @param freq_khz - the frequency of the IR carrier wave, unit KHz.
     * @param data_hex - a string in hex format, e.g. FFBBCC0011
     * 
     * @return bool 
     */
    bool write_data_hex_in_freq(uint16_t freq_khz, char *data_hex);
    
    /**
     * Set the protocol parameters. Please don't modify this only if you know what it is.
     * 
     * @param start_h - the length of start high signal
     * @param start_l - the length of start low signal
     * @param n_short - the length of short signal
     * @param n_long - the length of long signal
     * 
     * @return bool 
     */
    bool write_protocol_parameters(uint8_t start_h, uint8_t start_l, uint8_t n_short, uint8_t n_long);
    
    /**
     * Read the parameters of the IR protocol(NEC).
     * 
     * @param start_h - the length of start high signal
     * @param start_l - the length of start low signal
     * @param n_short - the length of short signal
     * @param n_long - the length of long signal
     * 
     * @return bool 
     */
    bool read_protocal_parametersbool(uint8_t *start_h, uint8_t *start_l, uint8_t *n_short, uint8_t *n_long);
    
    char *get_last_error() { return error_desc; };
    
private:
    PWM_T *io;
    char *error_desc;
    uint8_t *data;
    uint16_t _khz;
    
    bool _extract_data_from_string(char *str);
    void Send(unsigned char *idata, unsigned char ifreq);
    void space(int time);
    void mark(int time);
    void sendRaw(unsigned int buf[], int len, int hz);
    void enableIROut(int khz);
};


#endif

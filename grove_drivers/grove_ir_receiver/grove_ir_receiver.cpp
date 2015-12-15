/*
 * grove_ir_receiver.cpp
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
#include "grove_ir_receiver.h"



GroveIRRecv::GroveIRRecv(int pin)
{
    this->io = (IO_T *)malloc(sizeof(IO_T));
    this->timer = (TIMER_T *)malloc(sizeof(TIMER_T));
    this->data = (uint8_t *)malloc(26);
    this->data_hex = (uint8_t *)malloc(40);
    
    memset(this->data, 0, 26);

    suli_pin_init(io, pin, SULI_INPUT);

    Clear();

    suli_timer_install(timer, 50, grove_ir_recv_timer_interrupt_handler, this, true);
}

bool GroveIRRecv::read_last_data_recved(uint16_t *len, char **p_data)
{
    if (new_data_available)
    {
        *len = data[D_DATALEN];
        *p_data = data_hex;
        new_data_available = false;
    } else
    {
        *len = 0;
        *p_data = "";
    }
    return true;
}

bool GroveIRRecv::read_protocol_parameters(uint8_t *start_h, uint8_t *start_l, uint8_t *n_short, uint8_t *n_long)
{
    *start_h = data[D_STARTH];
    *start_l = data[D_STARTL];
    *n_short = data[D_SHORT];
    *n_long = data[D_LONG];
    return true;
}

void GroveIRRecv::Clear()
{
    irparams.rcvstate = STATE_IDLE;
    irparams.rawlen = 0;
}

int GroveIRRecv::decode(decode_results *results)
{
    results->rawbuf = irparams.rawbuf;
    results->rawlen = irparams.rawlen;
    if (irparams.rcvstate != STATE_STOP)
    {
        return ERR;
    }
    // Throw away and start over
    Clear();
    return DECODED;
}

//if get some data from IR
uint8_t GroveIRRecv::IsDta()
{
    if (decode(&results))
    {
        int count       = results.rawlen;
        if (count < 20 || (count - 4) % 8 != 0)
        {
            //Serial1.print("IR GET BAD DATA!\r\n");
            Clear();        // Receive the next value
            return 0;
        }
        int count_data  = (count - 4) / 16;
#if __IR_RECV_DEBUG
        Serial1.printf("ir get data! cnt = %d\r\n", count_data);
#endif
        return (uint8_t)(count_data + 6);
    } else
    {
        return 0;
    }

}

uint8_t GroveIRRecv::Recv()
{
    int count       = results.rawlen;
    int nshort      = 0;
    int nlong       = 0;
    int count_data  = 0;

    count_data = (count - 4) / 16;

    for (int i = 0; i < 10; i++)           // count nshort
    {
        nshort += results.rawbuf[3 + 2 * i];
    }
    nshort /= 10;

    int i = 0;
    int j = 0;
    while (1)        // count nlong
    {
        if (results.rawbuf[4 + 2 * i] > (2 * nshort))
        {
            nlong += results.rawbuf[4 + 2 * i];
            j++;
        }
        i++;
        if (j == 10) break;
        if ((4 + 2 * i) > (count - 10)) break;
    }
    nlong /= j;

    int doubleshort = 2 * nshort;
    for (i = 0; i < count_data; i++)
    {
        data[i + D_DATA] = 0x00;
        for (j = 0; j < 8; j++)
        {
            if (results.rawbuf[4 + 16 * i + j * 2] > doubleshort) // 1
            {
                data[i + D_DATA] |= 0x01 << (7 - j);
            } else
            {
                data[i + D_DATA] &= ~(0x01 << (7 - j));
            }
        }
    }
    data[D_LEN] = count_data + 5;
    data[D_STARTH] = results.rawbuf[1];
    data[D_STARTL] = results.rawbuf[2];
    data[D_SHORT] = nshort;
    data[D_LONG] = nlong;
    data[D_DATALEN] = count_data;

#if __IR_RECV_DEBUG
    Serial1.print("\r\n*************************************************************\r\n");
    Serial1.print("len\t = "); Serial1.println(data[D_LEN]);
    Serial1.print("start_h\t = "); Serial1.println(data[D_STARTH]);
    Serial1.print("start_l\t = "); Serial1.println(data[D_STARTL]);
    Serial1.print("short\t = "); Serial1.println(data[D_SHORT]);
    Serial1.print("long\t = "); Serial1.println(data[D_LONG]);
    Serial1.print("data_len = "); Serial1.println(data[D_DATALEN]);
    for (int i = 0; i < data[D_DATALEN]; i++)
    {
        Serial1.print(data[D_DATA + i]); Serial1.print("\t");
    }
    Serial1.print("\r\n*************************************************************\r\n");
#endif

    Clear(); // Receive the next value
    return data[D_LEN] + 1;
}

void GroveIRRecv::check_data()
{
    if (IsDta())
    {
        Recv();
        suli_event_trigger(event, &data[D_DATALEN], SULI_EDT_UINT8);
        _format_data();
        suli_event_trigger(event1, data_hex, SULI_EDT_STRING);
        new_data_available = true;
    }
}

void GroveIRRecv::_format_data()
{
    uint8_t len = data[D_DATALEN];
    if (len > 18)
    {
        return;
    }
    char tmp[3];
    int i;
    for (i = 0; i < len; i++)
    {
        snprintf(tmp, 3, "%02x", data[D_DATALEN + 1 + i]);
        memcpy(data_hex + 2 * i, tmp, 2);
    }
    data_hex[2 * i] = '\0';
}

EVENT_T* GroveIRRecv::attach_event_reporter_for_ir_recv_data_len(EVENT_CALLBACK_T reporter)
{
    this->event = (EVENT_T *)malloc(sizeof(EVENT_T));

    suli_event_init(event, reporter, NULL);

    return this->event;
}

EVENT_T* GroveIRRecv::attach_event_reporter_for_ir_recv_data_hex(EVENT_CALLBACK_T reporter)
{
    this->event1 = (EVENT_T *)malloc(sizeof(EVENT_T));

    suli_event_init(event1, reporter, NULL);

    return this->event1;
}


static void grove_ir_recv_timer_interrupt_handler(void *para)
{
    GroveIRRecv *g = (GroveIRRecv *)para;

    uint8_t irdata = suli_pin_read(g->io);

    g->irparams.timer++; // One more 50us tick
    if (g->irparams.rawlen >= RAWBUF)
    {
        // Buffer overflow
        g->irparams.rcvstate = STATE_STOP;
    }
    switch (g->irparams.rcvstate)
    {
    case STATE_IDLE: // In the middle of a gap
        if (irdata == MARK)
        {
            if (g->irparams.timer < GAP_TICKS)
            {
                // Not big enough to be a gap.
                g->irparams.timer = 0;
            } else
            {
                // gap just ended, record duration and start recording transmission
                g->irparams.rawlen = 0;
                g->irparams.rawbuf[g->irparams.rawlen++] = g->irparams.timer;
                g->irparams.timer = 0;
                g->irparams.rcvstate = STATE_MARK;
            }
        }
        break;
    case STATE_MARK: // timing MARK
        if (irdata == SPACE)   // MARK ended, record time
        {
            g->irparams.rawbuf[g->irparams.rawlen++] = g->irparams.timer;
            g->irparams.timer = 0;
            g->irparams.rcvstate = STATE_SPACE;
        }
        break;
    case STATE_SPACE: // timing SPACE
        if (irdata == MARK) // SPACE just ended, record it
        {
            g->irparams.rawbuf[g->irparams.rawlen++] = g->irparams.timer;
            g->irparams.timer = 0;
            g->irparams.rcvstate = STATE_MARK;
        } else // SPACE
        {
            if (g->irparams.timer > GAP_TICKS)
            {
                // big SPACE, indicates gap between codes
                // Mark current code as ready for processing
                // Switch to STOP
                // Don't reset timer; keep counting space width
                g->irparams.rcvstate = STATE_STOP;
            }
        }
        break;
    case STATE_STOP: // waiting, measuring gap
        if (irdata == MARK) // reset gap timer
        {
            g->irparams.timer = 0;
        }
        break;
    }

    g->check_data();

}

/*
 main.cpp - platform initialization and context switching
 emulation

 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

//This may be used to change user task stack size:
//#define CONT_STACKSIZE 4096
#include <Arduino.h>
extern "C" {
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "cont.h"
}

#define LOOP_TASK_PRIORITY 0
#define LOOP_QUEUE_SIZE    1

#define OPTIMISTIC_YIELD_TIME_US 16000

int atexit(void (*func)()) {
    return 0;
}

extern "C" void ets_update_cpu_frequency(int freqmhz);
void initVariant() __attribute__((weak));
void initVariant() {
}

extern void loop();
extern void setup();

void preloop_update_frequency() __attribute__((weak));
void preloop_update_frequency() {
#if defined(F_CPU) && (F_CPU == 160000000L)
    REG_SET_BIT(0x3ff00014, BIT(0));
    ets_update_cpu_frequency(160);
#endif
}

/**
 * init cpp stuff
 */
extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);
void do_global_ctors(void) {
    void(**p)(void);
    for(p = &__init_array_start; p != &__init_array_end; ++p) (*p)();
}

cont_t g_cont __attribute__ ((aligned (16)));
static os_event_t g_loop_queue[LOOP_QUEUE_SIZE];

static uint32_t g_micros_at_task_start;

extern "C" uint32_t esp_micros_at_task_start() {
    return g_micros_at_task_start;
}

extern "C" void abort() {
    do {
        *((int*)0) = 0;
    } while(true);
}

extern "C" void esp_yield() {
    if (cont_can_yield(&g_cont)) {
        cont_yield(&g_cont);
    }
}

extern "C" void esp_schedule() {
    system_os_post(LOOP_TASK_PRIORITY, 0, 0);
}

extern "C" void __yield() {
    if (cont_can_yield(&g_cont)) {
        esp_schedule();
        esp_yield();
    }
    else {
        abort();
    }
}

extern "C" void yield(void) __attribute__ ((weak, alias("__yield")));

extern "C" void optimistic_yield(uint32_t interval_us) {
    if (cont_can_yield(&g_cont) &&
        (system_get_time() - g_micros_at_task_start) > interval_us)
    {
        yield();
    }
}

extern void pre_user_loop();
extern void pre_user_setup();

void loop_wrapper() {
    static bool setup_done = false;
    if(!setup_done) {
        pre_user_setup();
        setup();
        setup_done = true;
    }
    preloop_update_frequency();
    pre_user_loop();
    loop();
    esp_schedule();
}

void loop_task(os_event_t *events) {
    g_micros_at_task_start = system_get_time();
    cont_run(&g_cont, &loop_wrapper);
    if(cont_check(&g_cont) != 0) {
        ets_printf("\r\nheap collided with sketch stack\r\n");
        abort();
    }
}



void init_done() {
    do_global_ctors();
    esp_schedule();
}


void arduino_init(void) {
    uart_div_modify(0, UART_CLK_FREQ / (115200));

    init();  //disable uart0 debug, init wiring system: pins, timer1

    initVariant();

    cont_init(&g_cont);

    system_os_task(loop_task, LOOP_TASK_PRIORITY, g_loop_queue, LOOP_QUEUE_SIZE);

    system_init_done_cb(&init_done);
}



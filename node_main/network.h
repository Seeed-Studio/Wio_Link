/*
 * network.h
 *
 * Copyright (c) 2012 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Jack Shao (jacky.shaoxg@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __NETWORK111_H__
#define __NETWORK111_H__

#include "esp8266.h"
#include "Arduino.h"
#include "circular_buffer.h"

enum
{
    WAIT_CONFIG, WAIT_GET_IP, DIED_IN_GET_IP, WAIT_RESOLVE, DIED_IN_RESOLVE, WAIT_CONN_DONE, DIED_IN_CONN, CONNECTED, WAIT_HELLO_DONE, KEEP_ALIVE, DIED_IN_HELLO
};

enum
{
    E_OK, E_FULL, E_NOT_READY
};
extern uint8_t conn_status[2];
extern struct espconn tcp_conn[2];
extern bool should_enter_user_loop;

void network_setup();
void network_normal_mode(int config_flag);
void network_config_mode();
int network_putc(CircularBuffer *tx_buffer, char c);
int network_puts(CircularBuffer *tx_buffer, char *data, int len);

extern CircularBuffer *data_stream_rx_buffer;
extern CircularBuffer *data_stream_tx_buffer;
extern CircularBuffer *ota_stream_rx_buffer ;
extern CircularBuffer *ota_stream_tx_buffer ;

extern uint32_t keepalive_last_recv_time[2];

bool extract_ip(uint8_t *input, uint8_t *output);
void format_server_address(uint8_t *input, uint8_t *output);
void fire_reboot(void *arg);

///
void start_resolving(void *arg);
void dns_resolved_callback(const char *name, ip_addr_t *ipaddr, void *callback_arg);

#endif

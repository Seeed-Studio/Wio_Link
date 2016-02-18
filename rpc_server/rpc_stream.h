/*
 * rpc_stream.h
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


#ifndef __STREAMxxx_H__
#define __STREAMxxx_H__

#include "suli2.h"
#include "rpc_server.h"

enum
{
    STREAM_DATA, STREAM_CMD
};

void stream_init(int stream_num);

char stream_read(int stream_num);

int stream_write(int stream_num, char c);
int stream_write_string(int stream_num, char *str, int len);

int stream_available(int stream_num);

int stream_print(int stream_num, type_t type, const void *data);
//data stream print
int writer_print(type_t type, const void *data);  
//data stream print, blocking model, this function can not be call in an ISR
int writer_block_print(type_t type, const void *data);  

void response_msg_open(int stream_num, char *msg_type);
void response_msg_append_400(int stream_num);
void response_msg_append_404(int stream_num);
void response_msg_close(int stream_num);

#endif

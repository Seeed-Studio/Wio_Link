/*
 * rpc_stream.cpp
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
#include "Arduino.h"
#include "rpc_stream.h"
#include "network.h"
#include "esp8266.h"

//extern Serial pc;

void stream_init(int stream_num)
{

}

char stream_read(int stream_num)
{
    CircularBuffer *rx_buffer = (stream_num == STREAM_DATA) ? data_stream_rx_buffer : ota_stream_rx_buffer;
    
    if (rx_buffer->size() > 0)
    {
        InterruptLock lock;
        char c;
        rx_buffer->read(&c,1);
        return c;
    } else return NULL;
}

int stream_write(int stream_num, char c)
{
    CircularBuffer *tx_buffer = (stream_num == STREAM_DATA) ? data_stream_tx_buffer : ota_stream_tx_buffer;
    return network_putc(tx_buffer, c);
}

int stream_available(int stream_num)
{
    CircularBuffer *rx_buffer = (stream_num == STREAM_DATA) ? data_stream_rx_buffer : ota_stream_rx_buffer;
    
    size_t sz;
    {
        InterruptLock lock;
        sz = rx_buffer->size();
    }
    return sz;
}

int stream_write_string(int stream_num, char *str, int len)
{
    CircularBuffer *tx_buffer = (stream_num == STREAM_DATA) ? data_stream_tx_buffer : ota_stream_tx_buffer;
    return network_puts(tx_buffer, str, len);
}

int stream_print(int stream_num, type_t type, const void *data)
{
    char buff[32];
    int len;
    
    if (data == NULL || type == TYPE_NONE) 
    {
        return E_NOT_READY;
    }
    
    switch (type)
    {
    case TYPE_BOOL:
    case TYPE_UINT8:
        sprintf(buff, "%u", *(uint8_t *)data);
        break;
    case TYPE_UINT16:
        sprintf(buff, "%u", *(uint16_t *)data);
        break;
    case TYPE_UINT32:
        sprintf(buff, "%lu", *(uint32_t *)data);
        break;
    case TYPE_INT8:
        sprintf(buff, "%d", *(int8_t *)data);
        break;
    case TYPE_INT:
        sprintf(buff, "%d", *(int *)data);
        break;
    case TYPE_INT16:
        sprintf(buff, "%d", *(int16_t *)data);
        break;
    case TYPE_INT32:
        sprintf(buff, "%ld", *(int32_t *)data);
        break;
    case TYPE_FLOAT:
        //sprintf(buff, "%f", *(float *)data);
        dtostrf((*(float *)data), NULL, 2, buff);
        break;
    case TYPE_STRING:
        len = strlen(data);
        return stream_write_string(stream_num, (char *)data, len);
    default:
        break;
    }
    len = strlen(buff);
    return stream_write_string(stream_num, buff, len);
}

int writer_print(type_t type, const void *data)
{
    return stream_print(STREAM_DATA, type, data);
}

int writer_block_print(type_t type, const void *data)
{
    while (stream_print(STREAM_DATA, type, data) == E_FULL)
    {
        delay(1);
    }
    return E_OK;
}

void response_msg_open(int stream_num, char *msg_type)
{
    char *msg1 = "{\"msg_type\":\"";
    char *msg2 = "\", \"msg\":";

    stream_write_string(stream_num, msg1, strlen(msg1));
    stream_write_string(stream_num, msg_type, strlen(msg_type));
    stream_write_string(stream_num, msg2, strlen(msg2));
}

void response_msg_append_400(int stream_num)
{
    char *msg = ", \"status\": 400";
    stream_write_string(stream_num, msg, strlen(msg));
}

void response_msg_append_404(int stream_num)
{
    char *msg = ", \"status\": 404";
    stream_write_string(stream_num, msg, strlen(msg));
}


void response_msg_close(int stream_num)
{
    char *msg3 = "}\r\n";

    stream_write_string(stream_num, msg3, strlen(msg3));
}


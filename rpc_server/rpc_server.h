/*
 * rpc_server.h
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

#ifndef __RPC_SERVER_H__
#define __RPC_SERVER_H__

#include "suli2.h"

typedef enum
{
    TYPE_NONE, TYPE_BOOL, TYPE_UINT8, TYPE_INT8, TYPE_UINT16, TYPE_INT16, TYPE_INT, TYPE_UINT32, TYPE_INT32, TYPE_FLOAT, TYPE_STRING
}type_t;

typedef enum
{
    METHOD_READ, METHOD_WRITE, METHOD_INTERNAL
}method_dir_t;

enum
{
    PARSE_REQ_TYPE, PARSE_GROVE_NAME, PARSE_METHOD, CHECK_ARGS, PRE_PARSE_ARGS, PARSE_ARGS, PARSE_CALL, DIVE_INTO_OTA, WAIT_OTA_DONE, GET_APP_NUM
};

enum
{
    REQ_GET, REQ_POST, REQ_OTA, REQ_APP_NUM
};

enum
{
    FROM_DRIVER, FROM_USER_SPACE
};

enum
{
    POP_FROM_EXTERNAL_Q, POP_FROM_INTERNAL_Q
};

typedef bool (*method_ptr_t)(void *class_ptr, char *method_name, void *input);

#define MAX_INPUT_ARG_LEN               4
typedef struct resource_s
{
    char               *grove_name;
    char               *method_name;
    method_dir_t       rw;
    method_ptr_t       method_ptr;
    void               *class_ptr;
    uint8_t            arg_types[MAX_INPUT_ARG_LEN];
    struct resource_s  *next;
}resource_t;

typedef struct event_s
{
    struct event_s     *prev;
    struct event_s     *next;
    char               *event_name;
    //void               *event_data;
    union
    {
        uint8_t         raw[4];
        uint8_t         u8;
        uint16_t        u16;
        uint32_t        u32;
        bool            boolean;
        int8_t          s8;
        int16_t         s16;
        int32_t         s32;
        int             integer;
        float           f;
        char            *p_str;
        void            *ptr;
    }event_data;
    int                event_data_type;
}event_t;

void rpc_server_init();

void rpc_server_register_method(char *grove_name, char *method_name, method_dir_t rw, method_ptr_t ptr, void *class_ptr, uint8_t *arg_types);

void rpc_server_register_resources();

void rpc_server_register_plugins();

void rpc_server_unregister_all();

void rpc_server_loop();

int rpc_server_event_put_data(event_t *event, void *p_data, int event_data_type);

void rpc_server_event_report(char *event_name, void *event_data, int event_data_type);
void rpc_server_event_report_from_user(char *event_name, void *event_data, int event_data_type);

bool rpc_server_event_queue_pop(event_t *event);
bool rpc_server_event_queue_pop_from_internal(event_t *event);

int rpc_server_event_queue_size(int target = POP_FROM_EXTERNAL_Q);


#define ARG_BUFFER_LEN                  256
#define NAME_LEN                        33
#define CMD_ARG_BUFFER_LEN              64


class RPCStreamProcessor
{
public:
    RPCStreamProcessor(int buff_len, int stream_id);
    
    void process_stream();
    
private:
    int        stream_id;
    int        req_type;
    int        parse_stage;
    
    char       *buff;
    int        offset = 0;
    int        arg_index = 0;
    char       grove_name[NAME_LEN];
    char       method_name[NAME_LEN];
    char       ch;
    uint8_t    arg_buff[4 * MAX_INPUT_ARG_LEN];
    int        arg_offset;
    resource_t *p_resource;
    
};

#endif


/*
 * rpc_server.cpp
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

#include "stdlib.h"
#include "Arduino.h"
#include "rpc_stream.h"
#include "rpc_server.h"
#include "rpc_queue.h"
#include "network.h"
#include "ota.h"


resource_t *p_first_resource;
resource_t *p_cur_resource;

event_t *p_event_queue_head;
event_t *p_event_queue_tail;

static Queue<event_t *> event_q_external(100);
static Queue<event_t *> event_q_internal(100);

static int parse_stage_data;
static int parse_stage_cmd;

extern void print_well_known();
void drain_event_queue();

RPCStreamProcessor rpc_stream_processor_data(ARG_BUFFER_LEN, STREAM_DATA);
RPCStreamProcessor rpc_stream_processor_cmd(CMD_ARG_BUFFER_LEN, STREAM_CMD);

void rpc_server_init()
{
    //init rpc stream
    stream_init(STREAM_DATA);
    stream_init(STREAM_CMD);
    //init rpc server
    p_first_resource = p_cur_resource = NULL;
    p_event_queue_head = p_event_queue_tail = NULL;
    parse_stage_data = PARSE_REQ_TYPE;
    parse_stage_cmd = PARSE_REQ_TYPE;

    rpc_server_register_resources();
    rpc_server_register_plugins();
    //printf("rpc server init done!\n");

}


void rpc_server_register_method(char *grove_name, char *method_name, method_dir_t rw, method_ptr_t ptr, void *class_ptr, uint8_t *arg_types)
{
    resource_t *p_res = (resource_t*)malloc(sizeof(resource_t));
    if (!p_res) return;

    if (p_first_resource == NULL)
    {
        p_first_resource = p_res;
        p_cur_resource = p_res;
    } else
    {
        p_cur_resource->next = p_res;
        p_cur_resource = p_res;
    }

    p_cur_resource->grove_name  = grove_name;
    p_cur_resource->method_name = method_name;
    p_cur_resource->rw          = rw;
    p_cur_resource->method_ptr  = ptr;
    p_cur_resource->class_ptr   = class_ptr;
    p_cur_resource->next = NULL;
    memcpy(p_cur_resource->arg_types, arg_types, sizeof(p_cur_resource->arg_types));

}

void rpc_server_unregister_all()
{
    if (p_first_resource == NULL) 
    {
        return;
    }
    
    resource_t *p_res = p_first_resource;
    
    while (p_res)
    {
        delete p_res->class_ptr;
        resource_t *tmp = p_res;
        p_res = p_res->next;
        free(tmp);
    }
    p_first_resource = NULL;
}

resource_t* __find_resource(char *name, char *method, int req_type)
{
/*
    if (!p_first_resource)
    {                     
        return NULL;      
    }                     
*/
    
    resource_t *ptr;
    for (ptr = p_first_resource; ptr; ptr = ptr->next)
    {
        if (strncmp(name, ptr->grove_name, 33) == 0 && strncmp(method, ptr->method_name, 33) == 0
            && req_type == ptr->rw)
        {
            return ptr;
        }
    }
    return NULL;
}

int __convert_arg(uint8_t *arg_buff, void *buff, int type)
{
    int i;
    uint32_t ui;
    switch (type)
    {
        case TYPE_BOOL:
            {
                i = atoi((const char *)(buff));
                ui = abs(i);
                memcpy(arg_buff, &ui, sizeof(bool));
                return sizeof(bool);
                break;
            }
        case TYPE_UINT8:
            {
                i = atoi((const char *)(buff));
                ui = abs(i);
                memcpy(arg_buff, &ui, 1);
                return 1;
                break;
            }
        case TYPE_UINT16:
            {
                i = atoi((const char *)(buff));
                ui = abs(i);
                memcpy(arg_buff, &ui, 2);
                return 2;
                break;
            }
        case TYPE_UINT32:
            {
                int32_t l = atol((const char *)(buff));
                ui = abs(l);
                memcpy(arg_buff, &ui, 4);
                return 4;
                break;
            }
        case TYPE_INT8:
            {
                i = atoi((const char *)(buff));
                char c = i;
                memcpy(arg_buff, &c, 1);
                return 1;
                break;
            }
        case TYPE_INT16:
            {
                i = atoi((const char *)(buff));
                memcpy(arg_buff, &i, 2);
                return 2;
                break;
            }
        case TYPE_INT32:
            {
                int32_t l = atol((const char *)(buff));
                memcpy(arg_buff, &l, 4);
                return 4;
                break;
            }
        case TYPE_INT:
            {
                int l = atol((const char *)(buff));
                memcpy(arg_buff, &l, sizeof(int));
                return sizeof(int);
                break;
            }
        case TYPE_FLOAT:
            {
                float f = atof((const char *)(buff));
                memcpy(arg_buff, &f, sizeof(float));
                return sizeof(float);
                break;
            }
        case TYPE_STRING:
            {
                uint32_t ptr = (uint32_t)buff;
                memcpy(arg_buff, &ptr, 4);
                return 4;
                break;
            }
        default:
            break;
    }
    return 0;
}

RPCStreamProcessor::RPCStreamProcessor(int buff_len, int streamid)
{
    this->buff = (char *)malloc(buff_len);
    this->stream_id = streamid;
}

void RPCStreamProcessor::process_stream()
{
    while (stream_available(stream_id) > 0 || parse_stage == PARSE_CALL)
    {
        //Serial1.println(stream_available(stream_id));
        switch (parse_stage)
        {
        case PARSE_REQ_TYPE:
            {
                bool parsed_req_type = false;

                buff[0] = buff[1]; buff[1] = buff[2]; buff[2] = buff[3];
                buff[3] = stream_read(stream_id);

                if (memcmp(buff, "GET", 3) == 0 || memcmp(buff, "get", 3) == 0)
                {
                    req_type = REQ_GET;
                    parsed_req_type = true;
                    response_msg_open(stream_id, "resp_get");
                }

                if (memcmp(buff, "POST", 4) == 0 || memcmp(buff, "post", 4) == 0)
                {
                    req_type = REQ_POST;
                    parsed_req_type = true;
                    stream_read(stream_id);  //read " " out
                    response_msg_open(stream_id, "resp_post");
                }
                
                if (memcmp(buff, "OTA", 3) == 0 || memcmp(buff, "ota", 3) == 0)
                {
                    parse_stage = DIVE_INTO_OTA;
                    response_msg_open(stream_id, "ota_trig_ack");
                    stream_print(stream_id, TYPE_STRING, "null");
                    response_msg_close(stream_id);
                    break;
                }

                if (memcmp(buff, "APP", 3) == 0 || memcmp(buff, "app", 3) == 0)
                {
                    parse_stage = GET_APP_NUM;
                    response_msg_open(stream_id, "resp_app");
                    break;
                }

                if (parsed_req_type)
                {
                    ch = stream_read(stream_id);
                    if (ch != '/')
                    {
                        //error request format
                        stream_print(stream_id, TYPE_STRING, "\"BAD REQUEST: missing root:'/'.\"");
                        response_msg_append_404(stream_id);
                        response_msg_close(stream_id);
                    } else
                    {
                        parse_stage = PARSE_GROVE_NAME;
                        p_resource = NULL;
                        offset = 0;
                    }
                }
                break;
            }
        case PARSE_GROVE_NAME:
            {
                ch = stream_read(stream_id);
                if (ch == '\r' || ch == '\n')
                {
                    buff[offset] = '\0';
                    if (strcmp(buff, ".well-known") == 0)
                    {
                        //writer_print(TYPE_STRING, "\"/.well-known is not implemented\"");
                        print_well_known();
                        response_msg_close(stream_id);
                        parse_stage = PARSE_REQ_TYPE;
                    } else
                    {
                        stream_print(stream_id, TYPE_STRING, "\"BAD REQUEST: missing method name.\"");
                        response_msg_append_404(stream_id);
                        response_msg_close(stream_id);
                        parse_stage = PARSE_REQ_TYPE;
                    }
                } else if (ch != '/' && offset < (NAME_LEN-1))
                {
                    buff[offset++] = ch;
                } else
                {
                    buff[offset] = '\0';
                    memcpy(grove_name, buff, offset + 1);
                    while (ch != '/')
                    {
                        ch = stream_read(stream_id);
                    }
                    parse_stage = PARSE_METHOD;
                    offset = 0;
                }
                break;
            }
        case PARSE_METHOD:
            {
                ch = stream_read(stream_id);
                if (ch == '\r' || ch == '\n')
                {
                    buff[offset] = '\0';
                    memcpy(method_name, buff, offset + 1);
                    parse_stage = CHECK_ARGS;  //to check if req missing arg
                } else if (ch == '/')
                {
                    buff[offset] = '\0';
                    memcpy(method_name, buff, offset + 1);
                    parse_stage = PRE_PARSE_ARGS;
                } else if (offset >= (NAME_LEN-1))
                {
                    buff[offset] = '\0';
                    memcpy(method_name, buff, offset + 1);
                    while (ch != '/' && ch != '\r' && ch != '\n')
                    {
                        ch = stream_read(stream_id);
                    }
                    if (ch == '\r' || ch == '\n') parse_stage = CHECK_ARGS;
                    else parse_stage = PRE_PARSE_ARGS;
                } else
                {
                    buff[offset++] = ch;
                }
                break;
            }
        case CHECK_ARGS:
            {
                p_resource = __find_resource((char *)grove_name, (char *)method_name, req_type);
                if (!p_resource)
                {
                    stream_print(stream_id, TYPE_STRING, "\"METHOD NOT FOUND WHEN CHECK ARGS\"");
                    response_msg_append_404(stream_id);
                    response_msg_close(stream_id);
                    parse_stage = PARSE_REQ_TYPE;
                    break;
                }
                if (p_resource->arg_types[0] != TYPE_NONE)
                {
                    stream_print(stream_id, TYPE_STRING, "\"MISSING ARGS\"");
                    response_msg_append_404(stream_id);
                    response_msg_close(stream_id);
                    parse_stage = PARSE_REQ_TYPE;
                    break;
                }
                parse_stage = PARSE_CALL;
                break;
            }
        case PRE_PARSE_ARGS:
            {
                p_resource = __find_resource((char *)grove_name, (char *)method_name, req_type);
                if (!p_resource)
                {
                    stream_print(stream_id, TYPE_STRING, "\"METHOD NOT FOUND WHEN PARSE ARGS\"");
                    response_msg_append_404(stream_id);
                    response_msg_close(stream_id);
                    parse_stage = PARSE_REQ_TYPE;
                    break;
                }
                parse_stage = PARSE_ARGS;
                arg_index = 0;
                arg_offset = 0;
                offset = 0;
                break;
            }
        case PARSE_ARGS:
            {
                bool overlen = false;
                ch = stream_read(stream_id);
                if (ch == '\r' || ch == '\n' || ch == '/')
                {
                    buff[offset] = '\0';
                } else if (offset >= ARG_BUFFER_LEN)
                {
                    buff[offset] = '\0';
                    while (ch != '/' && ch != '\r' && ch != '\n')
                    {
                        ch = stream_read(stream_id);
                    }
                    overlen = true;
                } else
                {
                    buff[offset++] = ch;
                }

                if (ch == '/')
                {
                    char *p = buff;
                    int len = __convert_arg(arg_buff + arg_offset, p, p_resource->arg_types[arg_index++]);
                    arg_offset += len;
                    offset = 0;
                }
                if (ch == '\r' || ch == '\n' || overlen)
                {
                    if ((arg_index < 3 && p_resource->arg_types[arg_index + 1] != TYPE_NONE) ||
                        (arg_index <= 3 && p_resource->arg_types[arg_index] != TYPE_NONE && strlen(buff) < 1))
                    {
                        stream_print(stream_id, TYPE_STRING, "\"MISSING ARGS\"");
                        response_msg_append_404(stream_id);
                        response_msg_close(stream_id);
                        parse_stage = PARSE_REQ_TYPE;
                        break;
                    }
                    char *p = buff;
                    int len = __convert_arg(arg_buff + arg_offset, p, p_resource->arg_types[arg_index++]);
                    arg_offset += len;
                    offset = 0;
                    parse_stage = PARSE_CALL;
                }
                break;
            }
        case PARSE_CALL:
            {
                if (!p_resource) p_resource = __find_resource((char *)grove_name, (char *)method_name, req_type);

                if (!p_resource)
                {
                    stream_print(stream_id, TYPE_STRING, "\"METHOD NOT FOUND WHEN CALL\"");
                    response_msg_close(stream_id);
                    parse_stage = PARSE_REQ_TYPE;
                    break;
                }
                //writer_print(TYPE_STRING, "{");
                if (false == p_resource->method_ptr(p_resource->class_ptr, p_resource->method_name, arg_buff)) response_msg_append_400(stream_id);
                //writer_print(TYPE_STRING, "}");
                response_msg_close(stream_id);

                parse_stage = PARSE_REQ_TYPE;
                break;
            }
        case DIVE_INTO_OTA:
            {
                //TODO: refer to the ota related code here
                ch = stream_read(stream_id);
                while (ch != '\r' && ch != '\n')
                {
                    ch = stream_read(stream_id);
                }
                //espconn_disconnect(&tcp_conn[0]);
                parse_stage = PARSE_REQ_TYPE;
                ota_start();
                while (!ota_fini)
                {
                    digitalWrite(STATUS_LED, ~digitalRead(STATUS_LED));
                    delay(100);
                    keepalive_last_recv_time[0] = keepalive_last_recv_time[1] = millis();  //to prevent online check and offline-reconnect during ota
                }
                break;
            }
        case GET_APP_NUM:
            {
                ch = stream_read(stream_id);
                while (ch != '\r' && ch != '\n')
                {
                    ch = stream_read(stream_id);
                }
                parse_stage = PARSE_REQ_TYPE;

                int bin_num = 1;
                if(system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
                {
                    Serial1.printf("Running user1.bin \r\n\r\n");
                    //os_memcpy(user_bin, "user2.bin", 10);
                    bin_num = 1;
                } else if(system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
                {
                    Serial1.printf("Running user2.bin \r\n\r\n");
                    //os_memcpy(user_bin, "user1.bin", 10);
                    bin_num = 2;
                }
                stream_print(stream_id, TYPE_INT, &bin_num);
                response_msg_close(stream_id);

                break;
            }
            
        default:
            break;
        }
        delay(0);  //yield the cpu for critical use or the watch dog will be hungry enough to trigger the reset.
    }
}


void rpc_server_loop()
{
    drain_event_queue();
    rpc_stream_processor_data.process_stream();
    rpc_stream_processor_cmd.process_stream();
}


static event_t event;

void drain_event_queue()
{
    int cnt = 0;
    /* report event if event queue is not empty */
    while (rpc_server_event_queue_pop(&event) && cnt++ < 5)
    {
        response_msg_open(STREAM_DATA,"event");
        writer_print(TYPE_STRING, "{\"");
        writer_print(TYPE_STRING, event.event_name);
        writer_print(TYPE_STRING, "\":\"");
        if (event.event_data_type != TYPE_STRING)
        {
            writer_print((type_t)event.event_data_type, &event.event_data);
        } else
        {
            writer_print((type_t)event.event_data_type, event.event_data.ptr);
        }
        writer_print(TYPE_STRING, "\"}");
        response_msg_close(STREAM_DATA);
        keepalive_last_recv_time[0] = millis();
        optimistic_yield(100);
    }
}

int rpc_server_event_put_data(event_t *event, void *p_data, int event_data_type)
{
    int sz = 0;
    
    memset(event->event_data.raw, 0, 4);
    event->event_data_type = event_data_type;
    
    switch (event_data_type)
    {
    case TYPE_BOOL:
        sz = sizeof(bool);
        break;
    case TYPE_UINT8:
        sz = sizeof(uint8_t);
        break;
    case TYPE_INT8:
        sz = sizeof(int8_t);
        break;
    case TYPE_UINT16:
        sz = sizeof(uint16_t);
        break;
    case TYPE_INT16:
        sz = sizeof(int16_t);
        break;
    case TYPE_INT:
        sz = sizeof(int);
        break;
    case TYPE_UINT32:
        sz = sizeof(uint32_t);
        break;
    case TYPE_INT32:
        sz = sizeof(int32_t);
        break;
    case TYPE_FLOAT:
        sz = sizeof(float);
        break;
    case TYPE_STRING:
        sz = sizeof(char *);
        event->event_data.p_str = p_data;
        return sz;
        break;
    case TYPE_NONE:
    default:
        break;
    }
    
    memcpy(event->event_data.raw, p_data, sz);
    return sz;
}

void _rpc_server_event_report(char *event_name, void *event_data, int event_data_type, int src)
{
    event_t *ev;

    ev = (event_t *)malloc(sizeof(event_t));
    if (ev)
    {
        ev->event_name = event_name;
        rpc_server_event_put_data(ev, event_data, event_data_type);
        
        {
            InterruptLock lock;
            if (!event_q_external.push(ev)) 
            {
                free(ev);
            }
        }
    }
    // copy event to internal event queue if the event comes from driver framework
    if (src == FROM_DRIVER)
    {
        ev = (event_t *)malloc(sizeof(event_t));
        if (ev)
        {
            ev->event_name = event_name;
            rpc_server_event_put_data(ev, event_data, event_data_type);
            
            {
                InterruptLock lock;
                if (!event_q_internal.push(ev)) 
                {
                    free(ev);
                }
            }
        }
    }
}

void rpc_server_event_report(char *event_name, void *event_data, int event_data_type)
{
    _rpc_server_event_report(event_name, event_data, event_data_type, FROM_DRIVER);
}
void rpc_server_event_report_from_user(char *event_name, void *event_data, int event_data_type)
{
    _rpc_server_event_report(event_name, event_data, event_data_type, FROM_USER_SPACE);
}

bool _rpc_server_event_queue_pop(event_t *event, int target)
{
    bool ret = false;
    Queue<event_t *> *p_q;
    if (target == POP_FROM_EXTERNAL_Q)
    {
        p_q = &event_q_external;
    } else
    {
        p_q = &event_q_internal;
    }
    
    {
        InterruptLock lock;
        if (p_q->get_size() == 0)
        {
            return ret;
        }
    }
    
    
    event_t *p_ev;
    
    {
        InterruptLock lock;
        if (p_q->pop(&p_ev)) 
        {
            memcpy(event, p_ev, sizeof(event_t));
            free(p_ev);
            ret = true;
        }
    }

    return ret;
}

bool rpc_server_event_queue_pop(event_t *event)
{
    return _rpc_server_event_queue_pop(event, POP_FROM_EXTERNAL_Q);
}
bool rpc_server_event_queue_pop_from_internal(event_t *event)
{
    return _rpc_server_event_queue_pop(event, POP_FROM_INTERNAL_Q);
}


int rpc_server_event_queue_size(int target)
{
    Queue<event_t *> *p_q;
    if (target == POP_FROM_EXTERNAL_Q)
    {
        p_q = &event_q_external;
    } else
    {
        p_q = &event_q_internal;
    }
    
    int sz;
    {
        InterruptLock lock;
        sz = p_q->get_size();
    }
    
    return sz;
}








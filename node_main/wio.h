/*
 * wio.h
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
#ifndef __WIO_H_HASHCODE__
#define __WIO_H_HASHCODE__

#include "esp8266.h"
#include "suli2.h"

#include "rpc_server.h"
#include "rpc_stream.h"



typedef struct variable_s
{
    char               *var_name;
    void               *var_ptr;
    uint8_t            var_type;
    struct variable_s  *next;
}variable_t;

typedef void (*rpc_function_t) (String);
typedef struct function_s
{
    char               *func_name;
    rpc_function_t     func_ptr;
    struct function_s  *next;
}function_t;


bool __plugin_variable_read(void *class_ptr, char *method_name, void *input_pack);
bool __plugin_variable_write(void *class_ptr, char *method_name, void *input_pack);

bool __plugin_function_write(void *class_ptr, char *method_name, void *input_pack);



class Wio
{
public:
    Wio();
    
    // event
    static void postEvent(char *event_name);
    static void postEvent(char *event_name, bool event_data);
    static void postEvent(char *event_name, uint8_t event_data);
    static void postEvent(char *event_name, int8_t event_data);
    static void postEvent(char *event_name, uint16_t event_data);
    static void postEvent(char *event_name, int16_t event_data);
    static void postEvent(char *event_name, uint32_t event_data);
    static void postEvent(char *event_name, int32_t event_data);
    static void postEvent(char *event_name, float event_data);
    static void postEvent(char *event_name, char *event_data);
    static void postEvent(char *event_name, String &event_data);
    
    static inline int eventAvailable()
    {
        return rpc_server_event_queue_size(POP_FROM_INTERNAL_Q);
    }
    
    static bool getEvent(event_t *p_event);
    
    // variable
    variable_t* findVariable(char *var_name);
    
    void registerVar(char *var_name, bool &var);
    void registerVar(char *var_name, uint8_t &var); 
    void registerVar(char *var_name, int8_t &var);  
    void registerVar(char *var_name, uint16_t &var);
    void registerVar(char *var_name, int16_t &var); 
    void registerVar(char *var_name, uint32_t &var);
    void registerVar(char *var_name, int32_t &var); 
    void registerVar(char *var_name, float &var);   
    void registerVar(char *var_name, char *var);
    void registerVar(char *var_name, String &event_data);
    
    // function
    function_t* findFunction(char *func_name);
    
    void registerFunc(char *func_name, rpc_function_t func);
    
    
    
private:
    variable_t *p_var_head;
    variable_t *p_var_cur;
    function_t *p_func_head;
    function_t *p_func_cur;
    
    void _registerVar(char *var_name, void *var_ptr, int var_type);
    

};

extern Wio wio;

#endif

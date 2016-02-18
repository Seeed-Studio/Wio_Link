/*
 * wio.cpp
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

#include "wio.h"
#include "base64.h"

Wio::Wio()
{ 
    p_var_head = p_var_cur = NULL;
    p_func_head = p_func_cur = NULL;
}

//---------------------- event --------------------------

static void Wio::postEvent(char *event_name)
{
    rpc_server_event_report_from_user(event_name, NULL, TYPE_NONE);
}

static void Wio::postEvent(char *event_name, bool event_data)
{
    rpc_server_event_report_from_user(event_name, &event_data, TYPE_BOOL);
}

static void Wio::postEvent(char *event_name, uint8_t event_data)
{
    rpc_server_event_report_from_user(event_name, &event_data, TYPE_UINT8);
}

static void Wio::postEvent(char *event_name, int8_t event_data)
{
    rpc_server_event_report_from_user(event_name, &event_data, TYPE_INT8);
}

static void Wio::postEvent(char *event_name, uint16_t event_data)
{
    rpc_server_event_report_from_user(event_name, &event_data, TYPE_UINT16);
}

static void Wio::postEvent(char *event_name, int16_t event_data)
{
    rpc_server_event_report_from_user(event_name, &event_data, TYPE_INT16);
}

static void Wio::postEvent(char *event_name, uint32_t event_data)
{
    rpc_server_event_report_from_user(event_name, &event_data, TYPE_UINT32);
}

static void Wio::postEvent(char *event_name, int32_t event_data)
{
    rpc_server_event_report_from_user(event_name, &event_data, TYPE_INT32);
}

static void Wio::postEvent(char *event_name, float event_data)
{
    rpc_server_event_report_from_user(event_name, &event_data, TYPE_FLOAT);
}

static void Wio::postEvent(char *event_name, char *event_data)
{
    rpc_server_event_report_from_user(event_name, event_data, TYPE_STRING);
}

static void Wio::postEvent(char *event_name, String &event_data)
{
    rpc_server_event_report_from_user(event_name, event_data.c_str(), TYPE_STRING);
}

static bool Wio::getEvent(event_t *p_event)
{
    return rpc_server_event_queue_pop_from_internal(p_event);
}


//---------------------- variable --------------------------
void Wio::_registerVar(char *var_name, void *var_ptr, int var_type)
{
    
    uint8_t arg_types[MAX_INPUT_ARG_LEN];

    //read func
    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);
    rpc_server_register_method("variable", var_name, METHOD_READ, __plugin_variable_read, this, arg_types);
    
    //write func
    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);
    arg_types[0] = var_type;
    rpc_server_register_method("variable", var_name, METHOD_WRITE, __plugin_variable_write, this, arg_types);
    
    variable_t *p_var = (variable_t*)malloc(sizeof(variable_t));
    if (!p_var) return;

    if (p_var_head == NULL)
    {
        p_var_head = p_var_cur = p_var;
    } else
    {
        p_var_cur->next = p_var;
        p_var_cur = p_var;
    }
    
    p_var_cur->var_name = var_name;
    p_var_cur->var_ptr = var_ptr;
    p_var_cur->var_type = var_type;
    p_var_cur->next = NULL;

}

variable_t* Wio::findVariable(char *var_name)
{
    if (!p_var_head)
    {
        return NULL;
    }
    
    variable_t *ptr;
    for (ptr = p_var_head; ptr; ptr = ptr->next)
    {
        if (strncmp(var_name, ptr->var_name, 33) == 0)
        {
            return ptr;
        }
    }
    return NULL;
}

void Wio::registerVar(char *var_name, bool &var)
{
    _registerVar(var_name, &var, TYPE_BOOL);
}

void Wio::registerVar(char *var_name, uint8_t &var)
{
    _registerVar(var_name, &var, TYPE_UINT8);
}

void Wio::registerVar(char *var_name, int8_t &var)
{
    _registerVar(var_name, &var, TYPE_INT8);
}
  
void Wio::registerVar(char *var_name, uint16_t &var)
{
    _registerVar(var_name, &var, TYPE_UINT16);
}

void Wio::registerVar(char *var_name, int16_t &var)
{
    _registerVar(var_name, &var, TYPE_INT16);
}
 
void Wio::registerVar(char *var_name, uint32_t &var)
{
    _registerVar(var_name, &var, TYPE_UINT32);
}

void Wio::registerVar(char *var_name, int32_t &var)
{
    _registerVar(var_name, &var, TYPE_INT32);
}
 
void Wio::registerVar(char *var_name, float &var)
{
    _registerVar(var_name, &var, TYPE_FLOAT);
}
   
void Wio::registerVar(char *var_name, char *var)
{
    _registerVar(var_name, var, TYPE_STRING);
}

void Wio::registerVar(char *var_name, String &var)
{
    _registerVar(var_name, var.c_str() , TYPE_STRING);
}




//---------------------- function --------------------------
function_t* Wio::findFunction(char *func_name)
{
    if (!p_func_head)
    {
        return NULL;
    }
    
    function_t *ptr;
    for (ptr = p_func_head; ptr; ptr = ptr->next)
    {
        if (strncmp(func_name, ptr->func_name, 33) == 0)
        {
            return ptr;
        }
    }
    return NULL;
}

void Wio::registerFunc(char *func_name, rpc_function_t func)
{
    uint8_t arg_types[MAX_INPUT_ARG_LEN];

    //write func
    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);
    arg_types[0] = TYPE_STRING;
    rpc_server_register_method("function", func_name, METHOD_WRITE, __plugin_function_write, this, arg_types);
    
    function_t *p_func = (function_t*)malloc(sizeof(function_t));
    if (!p_func) return;

    if (p_func_head == NULL)
    {
        p_func_head = p_func_cur = p_func;
    } else
    {
        p_func_cur->next = p_func;
        p_func_cur = p_func;
    }
    
    p_func_cur->func_name = func_name;
    p_func_cur->func_ptr = func;
    p_func_cur->next = NULL;
}
    
    
    
    
    
//------------------------------------------------
Wio wio;



//---------------------- function point for rpc server --------------------------
bool __plugin_variable_read(void *class_ptr, char *method_name, void *input_pack)
{
    Wio *w = (Wio *)class_ptr;
    
    variable_t *var = w->findVariable(method_name);
    
    if (var)
    {
        writer_print(TYPE_STRING, "\"");
        writer_print((type_t)var->var_type, var->var_ptr);
        writer_print(TYPE_STRING, "\"");
        
        return true;
    } else
    {
        writer_print(TYPE_STRING, "\"variable not registered\"");
        
        return false;
    }
    return true;
}


bool __plugin_variable_write(void *class_ptr, char *method_name, void *input_pack)
{
    Wio *w = (Wio *)class_ptr;
    
    variable_t *var = w->findVariable(method_name);
    
    if (var)
    {
        int sz = 0;
        
        switch (var->var_type)
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
            writer_print(TYPE_STRING, "\"string variable can not be written\"");
            return false;
            break;
        case TYPE_NONE:
        default:
            break;
        }
        memcpy(var->var_ptr, input_pack, sz);
        writer_print(TYPE_STRING, "\"OK\"");
        
        return true;
    } else
    {
        writer_print(TYPE_STRING, "\"variable not registered\"");
        
        return false;
    }
    return true;
}

bool __plugin_function_write(void *class_ptr, char *method_name, void *input_pack)
{
    Wio *w = (Wio *)class_ptr;
    
    function_t *f = w->findFunction(method_name);
    
    if (f)
    {
        uint8_t *arg_ptr = (uint8_t *)input_pack;
        
        char *pass_in_para;
        
        memcpy(&pass_in_para, arg_ptr, sizeof(char *)); 
        
        int len = strlen(pass_in_para);
        /*uint8_t *buf = (uint8_t *)malloc(len);
        
        if (!buf)
        {
            error_desc = "run out of memory";
            return false;
        }*/
        if (base64_decode(pass_in_para, &len, (const unsigned char *)pass_in_para, len) != 0)
        {
            writer_print(TYPE_STRING, "\"base64_decode error\"");
            return false;
        }
        pass_in_para[len] = '\0';
        
        String str(pass_in_para);
        
        f->func_ptr(str);
        
        writer_print(TYPE_STRING, "\"OK\"");
        
        return true;
    } else
    {
        writer_print(TYPE_STRING, "\"function not registered\"");
        
        return false;
    }
    return true;
}



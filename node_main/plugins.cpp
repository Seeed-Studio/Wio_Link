/*
 * plugins.cpp
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

#include "esp8266.h"
#include "suli2.h"
#include "rpc_server.h"
#include "rpc_stream.h"
#include "network.h"
#include "eeprom.h"

/** for data xchange channel */
bool __plugin_pm_sleep(void *class_ptr, char *method_name, void *input_pack)
{
    uint8_t *arg_ptr = (uint8_t *)input_pack;
    uint32_t sec;

    memcpy(&sec, arg_ptr, sizeof(uint32_t)); arg_ptr += sizeof(uint32_t);

    Serial1.printf("deep sleep %lu sec\r\n", sec);
    writer_print(TYPE_STRING, "\"ok, deep sleep\"");
    response_msg_close(STREAM_DATA);

    //turn off grove's power
    digitalWrite(SWITCH_GROVE_POWER, 0);

    uint32 cal1, cal2;
    cal1 = system_rtc_clock_cali_proc();
    Serial1.printf("cal 1  : %d.%d  \r\n", ((cal1*1000)>>12)/1000, ((cal1*1000)>>12)%1000);

    delay(100);
    system_deep_sleep(sec * 1000000);

    return true;
}

extern resource_t *p_first_resource;
static bool _grove_system_power_on = true;

bool __plugin_pm_power_the_groves(void *class_ptr, char *method_name, void *input_pack)
{
    uint8_t *arg_ptr = (uint8_t *)input_pack;
    uint8_t onoff;

    memcpy(&onoff, arg_ptr, sizeof(uint8_t)); arg_ptr += sizeof(uint8_t);

    Serial1.printf("power the groves: %d\r\n", onoff);
    if (onoff)
    {
        if (_grove_system_power_on)
        {
            return true;
        }

        digitalWrite(SWITCH_GROVE_POWER, 1);

        resource_t *p_res = p_first_resource;
        while (p_res)
        {
            if (p_res->rw == METHOD_INTERNAL && strncmp("?poweron", p_res->method_name, 9) == 0)
            {
                p_res->method_ptr(p_res->class_ptr, p_res->method_name, NULL);
            }
            p_res = p_res->next;
            delay(0);
        }

        //writer_print(TYPE_STRING, "\"ok, power on groves.\"");

        _grove_system_power_on = true;

    } else
    {
        if (_grove_system_power_on == false)
        {
            return true;
        }

        resource_t *p_res = p_first_resource;
        while (p_res)
        {
            if (p_res->rw == METHOD_INTERNAL && strncmp("?poweroff", p_res->method_name, 10) == 0)
            {
                if (!p_res->method_ptr(p_res->class_ptr, p_res->method_name, NULL))
                {
                    writer_print(TYPE_STRING, "\"");
                    writer_print(TYPE_STRING, p_res->grove_name);
                    writer_print(TYPE_STRING, " not support power down-n-up.\"");
                    return false;
                }
            }
            p_res = p_res->next;
            delay(0);
        }


        //writer_print(TYPE_STRING, "\"ok, power off groves.\"");

        digitalWrite(SWITCH_GROVE_POWER, 0);

        _grove_system_power_on = false;
    }

    return true;
}

/** for ota channel */

bool __plugin_setting_dataxserver(void *class_ptr, char *method_name, void *input_pack)
{
    uint8_t *arg_ptr = (uint8_t *)input_pack;
    char *ip;
    bool res = true;

    memcpy(&ip, arg_ptr, sizeof(char *)); arg_ptr += sizeof(char *);

    Serial1.printf("%s\r\n", ip);
    if (extract_ip(ip, EEPROM.getDataPtr() + EEP_DATA_SERVER_IP))
    {
        memcpy(EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR, ip, strlen(ip));
        memset(EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR + strlen(ip) , 0, 1);
        EEPROM.commit();
        stream_print(STREAM_CMD, TYPE_STRING, "\"ok\"");
        response_msg_close(STREAM_CMD);
        delay(100);
        fire_reboot(NULL);
    } else
    {
        stream_print(STREAM_CMD, TYPE_STRING, "\"failed, bad ip format.\"");
        return false;
    }

    return true;
}


void rpc_server_register_plugins()
{
    uint8_t arg_types[MAX_INPUT_ARG_LEN];

    //Power Management Plugin
    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);
    arg_types[0] = TYPE_UINT32;
    rpc_server_register_method("pm", "sleep", METHOD_WRITE, __plugin_pm_sleep, NULL, arg_types);
    //rpc_server_register_method("pm", "sleep_all", METHOD_WRITE, __plugin_pm_sleep_all, NULL, arg_types);
    arg_types[0] = TYPE_UINT8;
    rpc_server_register_method("pm", "power_grove", METHOD_WRITE, __plugin_pm_power_the_groves, NULL, arg_types);


    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);
    arg_types[0] = TYPE_STRING;
    rpc_server_register_method("setting", "dataxserver", METHOD_WRITE, __plugin_setting_dataxserver, NULL, arg_types);

}









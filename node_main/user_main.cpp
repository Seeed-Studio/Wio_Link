/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/1/23, v1.0 create this file.
*******************************************************************************/

#include "esp8266.h"
#include "Arduino.h"
#include "network.h"
#include "rpc_server.h"
#include "eeprom.h"

extern void arduino_init(void);

extern "C"
{
void system_phy_set_rfoption(uint8 option);
bool system_os_post(uint8 prio, os_signal_t sig, os_param_t par);
}


/**
 * This function is needed by new SDK 1.1.0 when linking stage
 *
 * @param
 */
extern "C"
void user_rf_pre_init()
{
    system_phy_set_rfoption(1);  //recalibrate the rf when power up
}


/**
 * this function will be linked by core_esp8266_main.cpp -
 * loop_wrapper
 *
 * @param
 */
void pre_user_setup()
{
    EEPROM.begin(4096);
    pinMode(FUNCTION_KEY, INPUT);
    pinMode(STATUS_LED, OUTPUT);
    pinMode(SWITCH_GROVE_POWER, OUTPUT);
    digitalWrite(SWITCH_GROVE_POWER, 1);
    network_setup();
    rpc_server_init();
}

/**
 * this function will be linked by core_esp8266_main.cpp - loop_wrapper
 *
 * @author Jack (5/23/2015)
 * @param
 */
void pre_user_loop()
{
    static bool smartconfig_pressed = false;
    static uint32_t smartconfig_pressed_time = 0;

    uint8_t v = digitalRead(FUNCTION_KEY);

    if(v == 0 && !smartconfig_pressed)
    {
        smartconfig_pressed_time = system_get_time();
        smartconfig_pressed = true;
    } else if(v == 0 && smartconfig_pressed)
    {
        if(system_get_time() - smartconfig_pressed_time > 2500000)
        {
            memset(EEPROM.getDataPtr() + EEP_OFFSET_CFG_FLAG, 1, 1);  //set the smart config flag
            EEPROM.commit();
            Serial1.println("will reboot to config mode");
            digitalWrite(STATUS_LED, 1);
            delay(100);
            digitalWrite(STATUS_LED, 0);
            delay(500);
            digitalWrite(STATUS_LED, 1);
            system_restart();
            smartconfig_pressed = false;
        }
    } else
    {
        smartconfig_pressed = false;
    }

    if(conn_status[0] == KEEP_ALIVE || conn_status[1] == KEEP_ALIVE)
    {
        rpc_server_loop();
    }
}

/**
 * Global function needed by libmain.a when linking
 *
 * @author Jack (5/23/2015)
 * @param
 */
extern "C"
void user_init(void)
{
    arduino_init();
}


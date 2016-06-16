/*
 * network.cpp
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
#include "Arduino.h"
#include "cbuf.h"
#include "network.h"
#include "eeprom.h"
#include "sha256.h"
#include "aes.h"

#define KEY_LEN             32
#define SN_LEN              32

static uint8_t get_ip_retry_cnt = 0;
uint8_t conn_status[2] = { WAIT_GET_IP, WAIT_GET_IP };
bool should_enter_user_loop = false;
static int conn_retry_cnt[2] = {0};
static uint16_t conn_retry_delay[2] = {1000, 1000};
static uint8_t get_hello[2] = {0};
static uint8_t confirm_hello_retry_cnt[2] = {0};
uint32_t keepalive_last_recv_time[2] = {0};

static struct espconn udp_conn;
struct espconn tcp_conn[2];
static struct _esp_tcp tcp_conn_tcp_s[2];
static os_timer_t timer_get_ip;
static os_timer_t timer_conn[2];
static os_timer_t timer_network_status_indicate[2];
static os_timer_t timer_confirm_hello[2];
static os_timer_t timer_tx[2];
static os_timer_t timer_keepalive_check[2];
const char *conn_name[2] = {"data", "ota" };

const char *device_find_request = "Node?";
const char *blank_device_find_request = "Blank?";
const char *device_keysn_write_req = "KeySN: ";
const char *ap_config_req = "APCFG: ";
const char *ap_change_req = "AP: ";
const char *reboot_req = "REBOOT";
const char *scan_req = "SCAN";
const char *version_req = "VERSION";

typedef struct __ap_info_s
{
    char ssid[32];
    struct __ap_info_s *next;
}__ap_info_t;
static volatile bool do_scan = false;
static uint8_t scan_cmd_source = 0;
static volatile bool can_scan = true;
static __ap_info_t *ap_info_list = NULL;

CircularBuffer *data_stream_rx_buffer = NULL;
CircularBuffer *data_stream_tx_buffer = NULL;
CircularBuffer *ota_stream_rx_buffer = NULL;
CircularBuffer *ota_stream_tx_buffer = NULL;

static aes_context aes_ctx_d[2];
static int iv_offset_d[2] = {0};
static unsigned char iv_d[2][16];
static aes_context aes_ctx_u[2];
static int iv_offset_u[2] = {0};
static unsigned char iv_u[2][16];
static bool txing[2] = {false};

extern "C" struct rst_info* system_get_rst_info(void);
void connection_init(void *arg);
void connection_send_hello(void *arg);
void connection_reconnect_callback(void *arg, int8_t err);
static void restart_network_initialization(void *arg);
void network_status_indicate_timer_fn(void *arg);

//////////////////////////////////////////////////////////////////////////////////////////

/**
 * format the SN string into safe-printing string when the node is first used
 * because the contents in Flash is random before valid SN is written.
 *
 * @param input
 * @param output
 */
static void format_sn(uint8_t *input, uint8_t *output)
{
    for (int i = 0; i < 32;i++)
    {
        if (*(input + i) < 33 || *(input + i) > 126)
        {
            *(output + i) = '#';
        } else *(output + i) = *(input + i);
    }
    *(output + 32) = '\0';
}

/**
 * validate if the server address has been written
 *
 * @param input
 *
 * @return bool
 */
static bool validate_server_address(uint8_t *input)
{
    bool ret = true;

    for (int i = 0; i < 100;i++)
    {
        if (*(input + i) == 0) break;

        if (*(input + i) < 33 || *(input + i) > 126)
        {
            ret = false;
            break;
        }
    }

    return ret;
}

/**
 * extract a subset of a string which is cutted with a \t charactor
 *
 * @param input
 * @param len
 * @param output
 */
static uint8_t *extract_substr(uint8_t *input, uint8_t *output)
{
    uint8_t *ptr = os_strchr(input, '\t');
    if (ptr == NULL)
    {
        return NULL;
    }

    os_memcpy(output, input, (ptr - input));

    char c = *(output + (ptr - input) - 1);
    if (c == '\r')
    {
        os_memset(output + (ptr - input) -1, 0, 1);
    } else
    {
        os_memset(output + (ptr - input), 0, 1);
    }
    return ptr;
}

/**
 * extract parts of ip v4 from a string
 *
 * @param input
 * @param output
 *
 * @return bool
 */
bool extract_ip(uint8_t *input, uint8_t *output)
{
    uint8_t *ptr;
    char num[4];
    int i = 0;

    while ((ptr = os_strchr(input, '.')) != NULL)
    {
        i++;
        os_memset(num, 0, 4);
        os_memcpy(num, input, (ptr - input));
        *output++ = atoi((const char *)num);
        input = ptr + 1;
    }

    if (i<3)
    {
        return false;
    } else
    {
        *output = atoi((const char *)input);
        return true;
    }
}

/**
 * format the server address to make sure http/https not inside
 *
 * @param input
 * @param output
 */
void format_server_address(uint8_t *input, uint8_t *output)
{
    uint8_t *ptr;

    if ((ptr = os_strstr(input, "://")) != NULL)
    {
        input = ptr + 3;
    }
    if ((ptr = os_strchr(input, '/')) != NULL)
    {
        *ptr = '\0';
    }
    os_memcpy(output, input, os_strlen(input)+1);
}

/**
 * print the data socket online status to ota socket stream
 */
void print_online_status()
{
    network_puts(ota_stream_tx_buffer, "{\"msg_type\":\"online_status\",\"msg\":", 34);
    if (conn_status[0] == KEEP_ALIVE)
    {
        network_puts(ota_stream_tx_buffer, "1", 1);
    } else
    {
        network_puts(ota_stream_tx_buffer, "0", 1);
    }
    network_puts(ota_stream_tx_buffer, "}\r\n", 3);
}

/**
 * perform a reboot
 */
void fire_reboot(void *arg)
{
    Serial1.println("fired reboot...");
    digitalWrite(STATUS_LED, 1);
    delay(100);
    digitalWrite(STATUS_LED, 0);
    delay(500);
    digitalWrite(STATUS_LED, 1);
    system_restart();
}

/**
 * Parse the message and configure node
 *
 * @param source - udp conn (0) or serial (1)
 * @param pusrdata
 * @param length
 */
static void parse_config_message(int source, char *pusrdata, unsigned short length, char *hwaddr, char *ip)
{
    if (pusrdata == NULL) {
        return;
    }

    //Serial1.println(pusrdata);

    uint8_t config_flag = *(EEPROM.getDataPtr() + EEP_OFFSET_CFG_FLAG);

    char *pkey;

    if ((os_strstr(pusrdata, device_find_request) != NULL) ||
        (os_strstr(pusrdata, blank_device_find_request) != NULL && config_flag == 2))
    {
        char *device_desc = new char[128];
        char *buff_sn = new char[33];
        format_sn(EEPROM.getDataPtr() + EEP_OFFSET_SN, (uint8_t *)buff_sn);
        os_sprintf(device_desc, "Node: %s," MACSTR "," IPSTR "\r\n",
                   buff_sn, MAC2STR(hwaddr), IP2STR(ip));

        Serial1.printf("%s", device_desc);
        length = os_strlen(device_desc);
        if (source == 0)
        {
            espconn_sendto(&udp_conn, device_desc, length);
        }
        if (source == 1)
        {
            Serial.print(device_desc);
        }

        delete[] device_desc;
        delete[] buff_sn;
    }
    else if ((pkey = os_strstr(pusrdata, reboot_req)) != NULL && config_flag == 2)
    {
        os_timer_disarm(&timer_conn[0]);
        os_timer_setfn(&timer_conn[0], fire_reboot, NULL);
        os_timer_arm(&timer_conn[0], 1, 0);
    }
    else if ((pkey = os_strstr(pusrdata, version_req)) != NULL && config_flag == 2)
    {
        if (source == 0)
        {
            espconn_sendto(&udp_conn, FW_VERSION, strlen(FW_VERSION));
            espconn_sendto(&udp_conn, "\r\n", 2);
        } else
        {
            Serial.print(FW_VERSION "\r\n");
        }
    }
    else if ((pkey = os_strstr(pusrdata, scan_req)) != NULL && config_flag == 2 && can_scan)
    {
        do_scan = true;
        scan_cmd_source = source;
    }
    else if ((pkey = os_strstr(pusrdata, ap_config_req)) != NULL && config_flag == 2)
    {
        //ssid
        pkey += os_strlen(ap_config_req);
        uint8_t *ptr;

        ptr = extract_substr(pkey, EEPROM.getDataPtr() + EEP_OFFSET_SSID);

        if (ptr)
        {
            Serial1.printf("Recv ssid: %s \r\n", EEPROM.getDataPtr() + EEP_OFFSET_SSID);
        } else
        {
            Serial1.printf("Bad format: wifi ssid password setting.\r\n");
            return;
        }

        //password
        ptr = extract_substr(ptr + 1, EEPROM.getDataPtr() + EEP_OFFSET_PASSWORD);

        if (ptr)
        {
            Serial1.printf("Recv password: %s  \r\n", EEPROM.getDataPtr() + EEP_OFFSET_PASSWORD);
        } else
        {
            Serial1.printf("Bad format: wifi ssid password setting.\r\n");
            return;
        }

        //key
        ptr = extract_substr(ptr + 1, EEPROM.getDataPtr() + EEP_OFFSET_KEY);

        if (ptr)
        {
            Serial1.printf("Recv key: %s  \r\n", EEPROM.getDataPtr() + EEP_OFFSET_KEY);
        } else
        {
            Serial1.printf("Bad format: can not find key.\r\n");
            return;
        }

        //sn
        ptr = extract_substr(ptr + 1, EEPROM.getDataPtr() + EEP_OFFSET_SN);

        if (ptr)
        {
            Serial1.printf("Recv sn: %s\r\n", EEPROM.getDataPtr() + EEP_OFFSET_SN);
        } else
        {
            Serial1.printf("Bad format: can not find sn.\r\n");
            return;
        }

        //ip data
        ptr = extract_substr(ptr + 1, EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR + 100);

        if (ptr)
        {
            Serial1.printf("Recv data server addr: %s\r\n", EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR + 100);
            format_server_address(EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR + 100, EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR);
            Serial1.printf("Formatted data server addr: %s.\r\n", EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR);
        } else
        {
            Serial1.printf("Bad format: can not find data server ip.\r\n");
            return;
        }


        //ip ota
        ptr = extract_substr(ptr + 1, EEPROM.getDataPtr() + EEP_OTA_SERVER_ADDR + 100);

        if (ptr)
        {
            Serial1.printf("Recv ota server addr: %s\r\n", EEPROM.getDataPtr() + EEP_OTA_SERVER_ADDR+100);
            format_server_address(EEPROM.getDataPtr() + EEP_OTA_SERVER_ADDR + 100, EEPROM.getDataPtr() + EEP_OTA_SERVER_ADDR);
            Serial1.printf("Formatted ota server addr: %s.\r\n", EEPROM.getDataPtr() + EEP_OTA_SERVER_ADDR);
        } else
        {
            Serial1.printf("Bad format: can not find ota server ip.\r\n");
            return;
        }


        config_flag = 3;
        memset(EEPROM.getDataPtr() + EEP_OFFSET_CFG_FLAG, config_flag, 1);  //set the config flag
        EEPROM.commit();

        if (source == 0)
        {
            espconn_sendto(&udp_conn, "ok\r\n", 4);
            espconn_sendto(&udp_conn, "ok\r\n", 4);
            espconn_sendto(&udp_conn, "ok\r\n", 4);
        }
        if (source == 1)
        {
            Serial.print("ok\r\nok\r\nok\r\n");
        }

        os_timer_disarm(&timer_conn[0]);
        os_timer_setfn(&timer_conn[0], fire_reboot, NULL);
        os_timer_arm(&timer_conn[0], 1000, 0);

    }
    else if ((pkey = os_strstr(pusrdata, ap_change_req)) != NULL && config_flag == 2)
    {
        //ssid
        pkey += os_strlen(ap_change_req);
        uint8_t *ptr;

        ptr = extract_substr(pkey, EEPROM.getDataPtr() + EEP_OFFSET_SSID);

        if (ptr)
        {
            Serial1.printf("Recv ssid: %s \r\n", EEPROM.getDataPtr() + EEP_OFFSET_SSID);
        } else
        {
            Serial1.printf("Bad format: wifi ssid password setting.\r\n");
            return;
        }

        //password
        ptr = extract_substr(ptr + 1, EEPROM.getDataPtr() + EEP_OFFSET_PASSWORD);

        if (ptr)
        {
            Serial1.printf("Recv password: %s  \r\n", EEPROM.getDataPtr() + EEP_OFFSET_PASSWORD);
        } else
        {
            Serial1.printf("Bad format: wifi ssid password setting.\r\n");
            return;
        }


        config_flag = 3;
        memset(EEPROM.getDataPtr() + EEP_OFFSET_CFG_FLAG, config_flag, 1);  //set the config flag
        EEPROM.commit();

        if (source == 0)
        {
            espconn_sendto(&udp_conn, "ok\r\n", 4);
            espconn_sendto(&udp_conn, "ok\r\n", 4);
            espconn_sendto(&udp_conn, "ok\r\n", 4);
        }
        if (source == 1)
        {
            Serial.print("ok\r\nok\r\nok\r\n");
        }

        os_timer_disarm(&timer_conn[0]);
        os_timer_setfn(&timer_conn[0], fire_reboot, NULL);
        os_timer_arm(&timer_conn[0], 1000, 0);

    }
}

/**
 * UDP packet recv callback
 *
 * @param arg - the pointer to espconn struct
 * @param pusrdata - recved data
 * @param length - length of the recved data
 */
static void udp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    struct espconn *conn = (struct espconn *)arg;
    char hwaddr[7];

    struct ip_info ipconfig;

    remot_info *premot = NULL;

    espconn_get_connection_info(conn, &premot, 0);  // get sender data (source IP)
    os_memcpy(udp_conn.proto.udp->remote_ip, premot->remote_ip, 4);
    udp_conn.proto.udp->remote_port = premot->remote_port;

    wifi_get_macaddr(STATION_IF, hwaddr);
    hwaddr[6] = '\0';

    // shows correct source IP
    Serial1.printf("UDP remote: " IPSTR ":%d\n",  IP2STR(udp_conn.proto.udp->remote_ip), udp_conn.proto.udp->remote_port);

    // shows always the current device IP; never a broadcast address
    Serial1.printf("UDP local:  " IPSTR ":%d\n",  IP2STR(conn->proto.udp->local_ip), conn->proto.udp->local_port);

    parse_config_message(0, pusrdata, length, hwaddr, conn->proto.udp->local_ip);

}

static void send_ap_item(void *arg)
{
    uint32_t source = (uint32_t)arg;

    if (ap_info_list)
    {
        char ssid[34];
        Serial1.printf("%s \r\n", ap_info_list->ssid);
        snprintf(ssid, 34, "%s\r\n", ap_info_list->ssid);
        __ap_info_t *last_ap = ap_info_list;
        ap_info_list = ap_info_list->next;
        free(last_ap);


        if (source == 0)
        {
            os_timer_disarm(&timer_tx[0]);
            os_timer_setfn(&timer_tx[0], send_ap_item, source);
            os_timer_arm(&timer_tx[0], 100, 0);
            espconn_sendto(&udp_conn, ssid, strlen(ssid));
        } else
        {
            os_timer_disarm(&timer_tx[0]);
            os_timer_setfn(&timer_tx[0], send_ap_item, source);
            os_timer_arm(&timer_tx[0], 5, 0);
            Serial.write(ssid, strlen(ssid));
        }
    } else
    {
        can_scan = true;
        if (source == 0)
        {
            espconn_sendto(&udp_conn, "\r\n", 2);
        }
        else
        {
            Serial.print("\r\n");
        }
    }
}

/**
 * The callback when data sent out via main UDP socket
 *
 * @param arg
 */
static void udp_sent_cb(void *arg)
{
    if (can_scan == false)
    {
        os_timer_disarm(&timer_tx[0]);
        os_timer_setfn(&timer_tx[0], send_ap_item, 0);
        os_timer_arm(&timer_tx[0], 50, 0);
    }
}


/**
 * init UDP socket
 */
void local_udp_config_port_init(void)
{
    udp_conn.type = ESPCONN_UDP;
    udp_conn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    udp_conn.proto.udp->local_port = 1025;
    espconn_regist_recvcb(&udp_conn, udp_recv_cb);
    espconn_regist_sentcb(&udp_conn, udp_sent_cb);

    espconn_create(&udp_conn);
}

/**
 * The callback for data receiving of the main TCP socket
 *
 * @param arg
 * @param pusrdata
 * @param length
 */
static void connection_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    struct espconn *pespconn = (struct espconn *)arg;
    int index = (pespconn == (&tcp_conn[0])) ? 0 : 1;
    CircularBuffer *rx_buffer = (index == 0) ? data_stream_rx_buffer : ota_stream_rx_buffer;
    CircularBuffer *tx_buffer = (index == 0) ? data_stream_tx_buffer : ota_stream_tx_buffer;

    char *pstr = NULL;
    size_t room;

    switch (conn_status[index])
    {
    case WAIT_HELLO_DONE:
        if ((pstr = strstr(pusrdata, "sorry")) != NULL)
        {
            get_hello[index] = 2;
        } else
        {
            aes_init(&aes_ctx_u[index]);
            aes_init(&aes_ctx_d[index]);
            aes_setkey_enc(&aes_ctx_u[index], EEPROM.getDataPtr() + EEP_OFFSET_KEY, KEY_LEN*8);
            aes_setkey_enc(&aes_ctx_d[index], EEPROM.getDataPtr() + EEP_OFFSET_KEY, KEY_LEN*8);
            memcpy(iv_u[index], pusrdata, 16);
            memcpy(iv_d[index], pusrdata, 16);
            aes_crypt_cfb128(&aes_ctx_u[index], AES_DECRYPT, length - 16, &iv_offset_u[index], iv_u[index], pusrdata + 16, pusrdata);
            if (os_strncmp(pusrdata, "hello", 5) == 0)
            {
                get_hello[index] = 1;
            }
        }

        break;
    case KEEP_ALIVE:
        aes_crypt_cfb128(&aes_ctx_u[index], AES_DECRYPT, length, &iv_offset_u[index], iv_u[index], pusrdata, pusrdata);

        Serial1.printf("%s conn: ", conn_name[index]);
        Serial1.println(pusrdata);
        keepalive_last_recv_time[index] = millis();

        if (os_strncmp(pusrdata, "##PING##", 8) == 0 && rx_buffer)
        {
            if (index == 0)
            {
                network_puts(tx_buffer, "##ALIVE##\r\n", 11);
            } else
            {
                print_online_status();
            }

            return;
        }

        if (!rx_buffer) return;

        //Serial1.printf("recv %d data\n", length);
        room = rx_buffer->capacity()-rx_buffer->size();
        length = os_strlen(pusrdata);  //filter out the padding 0s
        if ( room > 0 )
        {
            rx_buffer->write(pusrdata, (room>length?length:room));
        }
        break;
    default:
        break;
    }
}

/**
 * The callback when data sent out via main TCP socket
 *
 * @param arg
 */
static void connection_sent_cb(void *arg)
{

}

/**
 * The callback when tx data has written into ESP8266's tx buffer
 *
 * @param arg
 */
static void connection_tx_write_finish_cb(void *arg)
{
    struct espconn *p_conn = (struct espconn *)arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;

    CircularBuffer *tx_buffer = (index == 0) ? data_stream_tx_buffer : ota_stream_tx_buffer;

    if (!tx_buffer) return;

    size_t size = tx_buffer->size();

    size_t size2 = size;
    if (size > 0)
    {
        txing[index] = true;
        size2 = (((size % 16) == 0) ? (size) : (size + (16 - size % 16)));  //damn, the python crypto library only supports 16*n block length
        char *data = (char *)malloc(size2);
        os_memset(data, 0, size2);
        tx_buffer->read(data, size);
        aes_crypt_cfb128(&aes_ctx_d[index], AES_ENCRYPT, size2, &iv_offset_d[index], iv_d[index], data, data);
        espconn_send(p_conn, data, size2);
        free(data);
    } else
    {
        txing[index] = false;
    }
}

/**
 * put a char into tx ring-buffer
 *
 * @param c - char to send
 */
int network_putc(CircularBuffer *tx_buffer, char c)
{
    return network_puts(tx_buffer, &c, 1);
}

/**
 * put a block of data into tx ring-buffer
 *
 * @param data
 * @param len
 */
int network_puts(CircularBuffer *tx_buffer, char *data, int len)
{
    int index = (tx_buffer == data_stream_tx_buffer) ? 0 : 1;
    int tx_threshold = (index == 0) ? 512 : 32;

    if (conn_status[index] != KEEP_ALIVE || !tx_buffer) return E_NOT_READY;
    if (tcp_conn[index].state > ESPCONN_NONE)
    {
        size_t size_now;
        {
            InterruptLock lock;
            size_t room = tx_buffer->capacity() - tx_buffer->size();
            if (len > room)
            {
                return E_FULL;
            }

            tx_buffer->write(data, len);
            size_now = tx_buffer->size();
        }

        if ((strstr(data, "\r\n") || size_now > tx_threshold) && !txing[index])
        {
            //os_timer_disarm(&timer_tx);
            //os_timer_setfn(&timer_tx, main_connection_sent_cb, &tcp_conn[0]);
            //os_timer_arm(&timer_tx, 1, 0);
            connection_tx_write_finish_cb(&tcp_conn[index]);
        }
    }
    return E_OK;
}

/**
 * the function which will be called when timer_keepalive_check fired
 * to check if the socket to server is alive
 */
static void keepalive_check_timer_fn(void *arg)
{
    struct espconn *p_conn = (struct espconn *)arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;

    if (millis() - keepalive_last_recv_time[index] > 70000)
    {
        Serial1.printf("%s conn no longer alive, reconnect...\r\n", conn_name[index]);
        connection_reconnect_callback(p_conn, 0);
    } else
    {
        os_timer_arm(&timer_keepalive_check[index], 10000, 0);
    }
}

/**
 * Timer function for checking the hello response from server
 *
 * @param arg
 */
static void connection_confirm_hello(void *arg)
{
    struct espconn *p_conn = (struct espconn *)arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;

    Serial1.printf("%s conn confirm hello: %d \n", conn_name[index], get_hello[index]);

    if (get_hello[index] == 1)
    {
        Serial1.printf("%s conn handshake done, keep-alive\n", conn_name[index]);
        conn_status[index] = KEEP_ALIVE;
        keepalive_last_recv_time[index] = millis();
        os_timer_disarm(&timer_keepalive_check[index]);
        os_timer_setfn(&timer_keepalive_check[index], keepalive_check_timer_fn, p_conn);
        os_timer_arm(&timer_keepalive_check[index], 1000, 0);

        uint8_t ota_result_flag = *(EEPROM.getDataPtr() + EEP_OTA_RESULT_FLAG);

        if (ota_result_flag == 1 && index == 1)
        {
            memset(EEPROM.getDataPtr() + EEP_OTA_RESULT_FLAG, 0, 1);
            EEPROM.commit();
            for (int i = 0; i < 2;i++)
            {
                network_puts(ota_stream_tx_buffer, "{\"msg_type\":\"ota_result\",\"msg\":\"success\"}\r\n", 43);
            }
        } else if(ota_result_flag == 2 && index == 1)
        {
            memset(EEPROM.getDataPtr() + EEP_OTA_RESULT_FLAG, 0, 1);
            EEPROM.commit();
            network_puts(ota_stream_tx_buffer, "{\"msg_type\":\"ota_result\",\"msg\":\"fail\"}\r\n", 40);
        }

        os_timer_disarm(&timer_get_ip);
        os_timer_arm(&timer_get_ip, 100, 0);

    } else if (get_hello[index] == 2)
    {
        Serial1.printf("%s conn handshake: sorry from server\n", conn_name[index]);
        conn_status[index] = DIED_IN_HELLO;
    } else
    {
        if (++confirm_hello_retry_cnt[index] > 60)
        {
            confirm_hello_retry_cnt[index] = 1;
            if (index == 0)
            {
                conn_status[0] = WAIT_GET_IP;
                os_timer_disarm(&timer_conn[0]);
                os_timer_setfn(&timer_conn[0], &restart_network_initialization, NULL);
                os_timer_arm(&timer_conn[0], 1000, 0);
                return;
            }
            else
            {
                conn_status[1] = WAIT_CONN_DONE;
                os_timer_disarm(&timer_conn[1]);
                os_timer_setfn(&timer_conn[1], connection_init, &tcp_conn[1]);
                os_timer_arm(&timer_conn[1], 1000, 0);
                return;
            }
        }

        if (confirm_hello_retry_cnt[index] % 10 == 0)
        {
            os_timer_setfn(&timer_confirm_hello[index], connection_send_hello, p_conn);
        } else
        {
            os_timer_setfn(&timer_confirm_hello[index], connection_confirm_hello, p_conn);
        }

        os_timer_arm(&timer_confirm_hello[index], 1000, 0);
    }
}

/**
 * start the handshake with server
 * node will send itself's sn number and a signature signed with
 * its private key to server
 * @param
 */
void connection_send_hello(void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    int index = (pespconn == (&tcp_conn[0])) ? 0 : 1;

    uint8_t hmac_hash[32];

    uint8_t *buff = os_malloc(4 + SN_LEN + 32);
    if (!buff)
    {
        conn_status[index] = DIED_IN_HELLO;
        return;
    }
    //EEPROM.getDataPtr() + EEP_OFFSET_SN
    buff[0] = '@';
    os_memcpy(buff + 1, FW_VERSION, strlen(FW_VERSION));
    os_memcpy(buff + 4, EEPROM.getDataPtr() + EEP_OFFSET_SN, SN_LEN);

    sha256_hmac(EEPROM.getDataPtr() + EEP_OFFSET_KEY, KEY_LEN, buff+4, SN_LEN, hmac_hash, 0);

    os_memcpy(buff + 4 + SN_LEN, hmac_hash, 32);

    espconn_send(pespconn, buff, 4+SN_LEN+32);

    os_free(buff);

    get_hello[index] = 0;
    os_timer_disarm(&timer_confirm_hello[index]);
    os_timer_setfn(&timer_confirm_hello[index], connection_confirm_hello, pespconn);
    os_timer_arm(&timer_confirm_hello[index], 100, 0);
}


/**
 * The callback when the TCP socket has been open and connected with server
 *
 * @param arg
 */
void connection_connected_callback(void *arg)
{
    struct espconn *p_conn = (struct espconn *)arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;

    Serial1.printf("%s conn connected\r\n", conn_name[index]);
    conn_retry_cnt[index] = 0;
    conn_retry_delay[index] = 1000;
    espconn_regist_recvcb(p_conn, connection_recv_cb);
    espconn_regist_sentcb(p_conn, connection_sent_cb);

    espconn_set_opt(p_conn, 0x0c);  //enable the 2920 write buffer, enable keep-alive detection

    //espconn_regist_write_finish(p_conn, connection_tx_write_finish_cb); // register write finish callback
    espconn_regist_sentcb(p_conn, connection_tx_write_finish_cb);

    /* send hello */
    confirm_hello_retry_cnt[index] = 0;
    connection_send_hello(p_conn);
    conn_status[index] = WAIT_HELLO_DONE;

}


/**
 * The callback when some error or exception happened with the TCP socket
 *
 * @param arg
 * @param err
 */
void connection_reconnect_callback(void *arg, int8_t err)
{
    struct espconn *p_conn = (struct espconn *)arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;

    Serial1.printf("%s conn err: %d, will reconnect...\n", conn_name[index], err);

    os_timer_disarm(&timer_conn[index]);
    os_timer_setfn(&timer_conn[index], connection_init, &tcp_conn[index]);
    os_timer_arm(&timer_conn[index], conn_retry_delay[index], 0);
    conn_status[index] = WAIT_CONN_DONE;
}


/**
 * confirm the connection is connected
 */
static void connection_confirm_timer_fn(void *arg)
{
    struct espconn *p_conn = (struct espconn *)arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;

    if (conn_status[index] == WAIT_CONN_DONE)
    {
        espconn_disconnect(&tcp_conn[index]);
        os_timer_disarm(&timer_conn[index]);
        os_timer_setfn(&timer_conn[index], connection_init, &tcp_conn[index]);
        os_timer_arm(&timer_conn[index], 1, 0);
    }
}

/**
 * init the TCP socket
 */
void connection_init(void *arg)
{

    struct espconn *p_conn = (struct espconn *)arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;
    char *server_ip = (index == 0) ? (EEPROM.getDataPtr() + EEP_DATA_SERVER_IP) : (EEPROM.getDataPtr() + EEP_OTA_SERVER_IP);

    Serial1.printf("%s connection init.\r\n", conn_name[index]);

    //cant connect to server after 1min, maybe should re-join the router to renew the IP too.
    if (++conn_retry_cnt[index] > 60)
    {
        conn_retry_cnt[index] = 1;
        if (index == 0)
        {
            conn_status[0] = WAIT_GET_IP;
            os_timer_disarm(&timer_conn[0]);
            os_timer_setfn(&timer_conn[0], &restart_network_initialization, NULL);
            os_timer_arm(&timer_conn[0], 1000, 0);
            return;
        }
        if (index == 1)
        {
            conn_retry_delay[1] = 10000;
        }
    }

    tcp_conn[index].type = ESPCONN_TCP;
    tcp_conn[index].state = ESPCONN_NONE;
    tcp_conn[index].proto.tcp = &tcp_conn_tcp_s[index];
    //const char server_ip[4] = SERVER_IP;
    os_memcpy(tcp_conn[index].proto.tcp->remote_ip, server_ip, 4);
    tcp_conn[index].proto.tcp->remote_port = (index == 0) ? DATA_SERVER_PORT : OTA_SERVER_PORT;
    tcp_conn[index].proto.tcp->local_port = espconn_port();
    espconn_regist_connectcb(&tcp_conn[index], connection_connected_callback);
    espconn_regist_reconcb(&tcp_conn[index], connection_reconnect_callback);
    espconn_connect(&tcp_conn[index]);

    os_timer_disarm(&timer_conn[index]);
    os_timer_setfn(&timer_conn[index], connection_confirm_timer_fn, &tcp_conn[index]);
    os_timer_arm(&timer_conn[index], 10000, 0);
}

void dns_resolved_callback(const char *name, ip_addr_t *ipaddr, void *callback_arg)
{
    struct espconn *p_conn = (struct espconn *)callback_arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;
    char *server_addr = (index == 0) ? (EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR) : (EEPROM.getDataPtr() + EEP_OTA_SERVER_ADDR);
    char *server_ip = (index == 0) ? (EEPROM.getDataPtr() + EEP_DATA_SERVER_IP) : (EEPROM.getDataPtr() + EEP_OTA_SERVER_IP);

    if (ipaddr == NULL)
    {
        Serial1.printf("%s Resolve failed\r\n", name);
    } else
    {
        Serial1.printf("%s Resolved IP: " IPSTR "\r\n", name, IP2STR(&ipaddr->addr));
        os_memcpy(server_ip, &ipaddr->addr, 4);
    }

    if (index == 0)
    {
        os_timer_disarm(&timer_conn[0]);
        os_timer_setfn(&timer_conn[0], &start_resolving, &tcp_conn[1]);
        os_timer_arm(&timer_conn[0], 1, 0);
    } else
    {
        conn_status[0] = WAIT_CONN_DONE;
        conn_status[1] = WAIT_CONN_DONE;

        /* establish the connection with server */
        connection_init(&tcp_conn[0]);
        connection_init(&tcp_conn[1]);
    }


}

/**
 * start resolving the ip address of ota / data server
 *
 */
void start_resolving(void *arg)
{
    struct espconn *p_conn = (struct espconn *)arg;
    int index = (p_conn == (&tcp_conn[0])) ? 0 : 1;
    char *server_addr = (index == 0) ? (EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR) : (EEPROM.getDataPtr() + EEP_OTA_SERVER_ADDR);
    char *server_ip = (index == 0) ? (EEPROM.getDataPtr() + EEP_DATA_SERVER_IP) : (EEPROM.getDataPtr() + EEP_OTA_SERVER_IP);


    Serial1.printf("Resolving %s...\r\n", server_addr);

    ip_addr_t addr;
    err_t err = espconn_gethostbyname(arg, server_addr, &addr, &dns_resolved_callback);
    if (err == ESPCONN_OK)
    {
        Serial1.printf("Resolved IP: " IPSTR "\r\n", IP2STR(&addr.addr));
        os_memcpy(server_ip, &addr.addr, 4);

        if (index == 0)
        {
            os_timer_disarm(&timer_conn[0]);
            os_timer_setfn(&timer_conn[0], &start_resolving, &tcp_conn[1]);
            os_timer_arm(&timer_conn[0], 1, 0);
        } else
        {
            conn_status[0] = WAIT_CONN_DONE;
            conn_status[1] = WAIT_CONN_DONE;

            /* establish the connection with server */
            connection_init(&tcp_conn[0]);
            connection_init(&tcp_conn[1]);
        }
    }
}

/**
 * timer function for checking if the ip address has been get
 *
 * @param arg
 */
static void check_getting_ip_address(void *arg)
{
    const char *get_ip_state[6] = {
        "IDLE",
        "CONNECTING",
        "WRONG_PASSWORD",
        "NO_AP_FOUND",
        "CONNECT_FAIL",
        "GOT_IP"
    };
    uint8_t connect_status = wifi_station_get_connect_status();

    if (connect_status != STATION_GOT_IP)
    {
        Serial1.printf("Waiting ip, state: %s\n", get_ip_state[connect_status]);

        if (++get_ip_retry_cnt > 60)
        {
            conn_status[0] = WAIT_GET_IP;
            os_timer_disarm(&timer_conn[0]);
            os_timer_setfn(&timer_conn[0], &restart_network_initialization, NULL);
            os_timer_arm(&timer_conn[0], 1000, 0);
        }
        return;
    }

    os_timer_disarm(&timer_conn[0]);

    struct ip_info ip;
    wifi_get_ip_info(STATION_IF, &ip);
    Serial1.printf("Done. IP: " IPSTR "\r\n", IP2STR(&ip.ip));

    /* open the config interface at UDP port 1025 */
    local_udp_config_port_init();

    bool valid_server_addr1 = validate_server_address(EEPROM.getDataPtr() + EEP_DATA_SERVER_ADDR);
    bool valid_server_addr2 = validate_server_address(EEPROM.getDataPtr() + EEP_OTA_SERVER_ADDR);
    if (valid_server_addr1 && valid_server_addr2)  //has been configured with new app at least once time.
    {
        conn_status[0] = WAIT_RESOLVE;
        conn_status[1] = WAIT_RESOLVE;

        start_resolving(&tcp_conn[0]);

    } else
    {
        conn_status[0] = WAIT_CONN_DONE;
        conn_status[1] = WAIT_CONN_DONE;

        /* establish the connection with server */
        connection_init(&tcp_conn[0]);
        connection_init(&tcp_conn[1]);
    }
}

/**
 * 10sec timeout when connecting to server, should enter user loop anyway
 *
 * @param arg
 */
static void timeout_connecting_server(void *arg)
{
    should_enter_user_loop = true;
}

/**
 * re-init the network
 *
 * @param arg
 */
static void restart_network_initialization(void *arg)
{
    network_normal_mode(0);
}

/**
 * begin to establish network
 */
void network_normal_mode(int config_flag)
{
    Serial1.printf("start to establish network connection.\r\n");

    /* enable the station mode */
    wifi_set_opmode(0x01);

    struct station_config config;
    wifi_station_get_config(&config);

    if (config_flag >= 2)
    {
        if (config_flag == 3)
        {
            os_strncpy(config.ssid, EEPROM.getDataPtr() + EEP_OFFSET_SSID, 32);
            os_strncpy(config.password, EEPROM.getDataPtr() + EEP_OFFSET_PASSWORD, 64);
            wifi_station_set_config(&config);
        }

        memset(EEPROM.getDataPtr() + EEP_OFFSET_CFG_FLAG, 0, 1);  //clear the config flag
        EEPROM.commit();

        Serial1.printf("Config flag has been reset to 0.\r\n");
    }

    Serial1.printf("connect to ssid %s with passwd %s\r\n", config.ssid, config.password);
    wifi_station_disconnect();
    wifi_station_connect(); //connect with saved config in flash

    /* start to check IP */
    get_ip_retry_cnt = 0;
    os_timer_disarm(&timer_conn[0]);
    os_timer_setfn(&timer_conn[0], check_getting_ip_address, NULL);
    os_timer_arm(&timer_conn[0], 1000, 1);

    /* wait 10 sec to connect server, or the user setup and loop will be involved */
    os_timer_disarm(&timer_get_ip);
    os_timer_setfn(&timer_get_ip, timeout_connecting_server, NULL);
    os_timer_arm(&timer_get_ip, 10000, 1);
}

static void ap_scan_done(void *arg, STATUS status) {
    if (status == OK) {
        struct bss_info *bss_link = (struct bss_info *)arg;
        int cnt = 0;
        __ap_info_t *ap_last;
        while (bss_link)
        {
            __ap_info_t *ap = (__ap_info_t *)malloc(sizeof(__ap_info_t));
            ap->next = NULL;
            memcpy(ap->ssid, bss_link->ssid, 32);
            if (cnt == 0)
            {
                ap_info_list = ap;
            } else
            {
                ap_last->next = ap;
            }
            bss_link = bss_link->next.stqe_next;
            ap_last = ap;
            cnt++;
        }
        Serial1.printf("cnt: %d\r\n", cnt);
        if (cnt > 0)
        {
            os_timer_disarm(&timer_tx[0]);
            os_timer_setfn(&timer_tx[0], send_ap_item, scan_cmd_source);
            os_timer_arm(&timer_tx[0], 1, 0);
        } else
        {
            can_scan = true;
        }
    }
}

/**
 * Enter AP mode then waiting to be configured
 */
void network_config_mode()
{
    Serial.begin(115200);

    Serial1.printf("enter AP mode ... \r\n");

    memset(EEPROM.getDataPtr() + EEP_OFFSET_CFG_FLAG, 2, 1);  //upgrade the config flag
    EEPROM.commit();

    wifi_set_opmode(0x03); //softAP + Station mode

    struct softap_config *config = (struct softap_config *)malloc(sizeof(struct softap_config));
    char hwaddr[7];
    char ssid[16];

    wifi_get_macaddr(SOFTAP_IF, hwaddr);
    wifi_softap_get_config_default(config);
    os_sprintf(ssid, "Wio_%02X%02X%02X", hwaddr[3], hwaddr[4], hwaddr[5]);
    memcpy(config->ssid, ssid, 15);
    config->ssid_len = strlen(config->ssid);
    wifi_softap_set_config(config);

    Serial.printf("SSID: %s\r\n", config->ssid);
    Serial1.printf("SSID: %s\r\n", config->ssid);

    /* open the config interface at UDP port 1025 */
    local_udp_config_port_init();

    /* list connected devices */
    uint32_t start_time0 = millis();
    struct station_info * station;
    char msg[512] = { 0 };
    int index = 0;
    while (1)
    {
        if (do_scan)
        {
            can_scan = false;
            do_scan = false;
            wifi_station_scan (NULL, &ap_scan_done);
        }
        while (Serial.available() > 0)
        {
            if (index > 510) index = 0;
            msg[index++] = (char)Serial.read();
            delay(0);
            if (os_strstr(msg, "\r\n") != NULL)
            {
                msg[index] = '\0';
                index = 9999;
                break;
            }
        }
        delay(10);
        if (index == 9999)
        {
            Serial1.println(msg);

            wifi_get_macaddr(STATION_IF, hwaddr);
            hwaddr[6] = '\0';
            parse_config_message(1, msg, os_strlen(msg), hwaddr, "");
            index = 0;
            os_memset(msg, 0, sizeof(msg));
        }
    }
}


/**
 * setup the network when system startup
 */
void network_setup()
{
#if ENABLE_DEBUG_ON_UART1
    Serial1.begin(74880);
    //Serial1.setDebugOutput(true);
    Serial1.println("\n\n");
#else
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, 1);
#endif

    if (!data_stream_rx_buffer) data_stream_rx_buffer = new CircularBuffer(512);
    if (!data_stream_tx_buffer) data_stream_tx_buffer = new CircularBuffer(1024);

    if (!ota_stream_rx_buffer) ota_stream_rx_buffer = new CircularBuffer(128);
    if (!ota_stream_tx_buffer) ota_stream_tx_buffer = new CircularBuffer(256);

    struct rst_info *reason = system_get_rst_info();
    const char *boot_reason_desc[7] = {
        "DEFAULT",
        "WDT",
        "EXCEPTION",
        "SOFT_WDT",
        "SOFT_RESTART",
        "DEEP_SLEEP_AWAKE",
        "EXT_SYS"
    };
    Serial1.printf("Boot reason: %s\n", boot_reason_desc[reason->reason]);
    Serial1.printf("Node name: %s\n", NODE_NAME);
    Serial1.printf("FW version: %s\n", FW_VERSION);
    Serial1.printf("Chip id: 0x%08x\n", system_get_chip_id());

    Serial1.print("Free heap size: ");
    Serial1.println(system_get_free_heap_size());

    /* get key and sn */
    char buff[33];

    //os_memcpy(EEPROM.getDataPtr() + EEP_OFFSET_KEY, "9ed12049da8c1eb42a9872bb27cfb02e", 32);
    format_sn(EEPROM.getDataPtr() + EEP_OFFSET_KEY, (uint8_t *)buff);
    Serial1.printf("Private key: %s\n", buff);

    //os_memcpy(EEPROM.getDataPtr() + EEP_OFFSET_SN, "52f4370b14aedea33416db677b1c5726", 32);
    format_sn(EEPROM.getDataPtr() + EEP_OFFSET_SN, (uint8_t *)buff);
    Serial1.printf("Node SN: %s\n", buff);

    //
    Serial1.printf("Data server: " IPSTR "\r\n", IP2STR((uint32 *)(EEPROM.getDataPtr() + EEP_DATA_SERVER_IP)));

    Serial1.printf("OTA server: " IPSTR "\r\n", IP2STR((uint32 *)(EEPROM.getDataPtr() + EEP_OTA_SERVER_IP)));

    Serial1.printf("OTA result flag: %d\r\n", *(EEPROM.getDataPtr() + EEP_OTA_RESULT_FLAG));

    Serial1.flush();
    delay(1000);  //should delay some time to wait all settled before wifi_* API calls.

    //start the forever timer to drive the status leds

#if !ENABLE_DEBUG_ON_UART1
    os_timer_disarm(&timer_network_status_indicate[0]);
    os_timer_setfn(&timer_network_status_indicate[0], network_status_indicate_timer_fn, NULL);
    os_timer_arm(&timer_network_status_indicate[0], 1, false);
#endif

    /* get the smart config flag */
    uint8_t config_flag = *(EEPROM.getDataPtr() + EEP_OFFSET_CFG_FLAG);

    if (config_flag == 1)
    {
        conn_status[0] = WAIT_CONFIG;
        network_config_mode();
    } else
    {
        conn_status[0] = WAIT_GET_IP;
        conn_status[1] = WAIT_GET_IP;
        network_normal_mode(config_flag);
    }
}

/**
 * Blink the LEDs according to the status of network connection
 */
#if !ENABLE_DEBUG_ON_UART1

void network_status_indicate_timer_fn(void *arg)
{
    static uint8_t last_main_status = 0xff;
    static uint16_t blink_pattern_cnt = 0;
    static uint8_t brightness_dir = 0;

    switch (conn_status[0])
    {
    case WAIT_CONFIG:
        //os_timer_arm(&timer_network_status_indicate[0], 100, false);
        //digitalWrite(STATUS_LED, ~digitalRead(STATUS_LED));
        os_timer_arm(&timer_network_status_indicate[0], 60, false);
        if (conn_status[0] != last_main_status)
        {
            blink_pattern_cnt = 0;
            brightness_dir = 0;
        }
        else if (brightness_dir == 0)
        {
            blink_pattern_cnt += 50;
            if (blink_pattern_cnt > 800)
            {
                blink_pattern_cnt = 800;
                brightness_dir = 1;
            }
        }
        else if (brightness_dir == 1)
        {
            blink_pattern_cnt -= 40;
            if (blink_pattern_cnt < 40)
            {
                blink_pattern_cnt = 0;
                brightness_dir = 0;
            }
        }
        analogWrite(STATUS_LED, 1023-blink_pattern_cnt);
        break;
    case WAIT_GET_IP:
    case WAIT_RESOLVE:
        if (conn_status[0] != last_main_status)
        {
            os_timer_arm(&timer_network_status_indicate[0], 50, false);
            digitalWrite(STATUS_LED, 0);
            blink_pattern_cnt = 1;
        }else
        {
            int timeout = 50;
            if (++blink_pattern_cnt >= 4)
            {
                timeout = 1000;
                blink_pattern_cnt = 0;
            }
            os_timer_arm(&timer_network_status_indicate[0], timeout, false);
            digitalWrite(STATUS_LED, ~digitalRead(STATUS_LED));
        }
        break;
    case DIED_IN_GET_IP:
    case DIED_IN_RESOLVE:
        digitalWrite(STATUS_LED, 0);
        break;
    case WAIT_CONN_DONE:
    case WAIT_HELLO_DONE:
        if (conn_status[0] != last_main_status)
        {
            os_timer_arm(&timer_network_status_indicate[0], 50, false);
            analogWrite(STATUS_LED, 0);  //stop timer1
            digitalWrite(STATUS_LED, 0);
        }else
        {
            os_timer_arm(&timer_network_status_indicate[0],(digitalRead(STATUS_LED) > 0 ? 50 : 1000), false);
            digitalWrite(STATUS_LED, ~digitalRead(STATUS_LED));
        }
        break;
    case DIED_IN_CONN:
        Serial1.printf("The main connection died after 5 retry\n");
        Serial1.println("Please check server's ip and port, also check AccessToken\n");
        digitalWrite(STATUS_LED, 0);
        break;
    case DIED_IN_HELLO:
        Serial1.printf("No hello ack from server after 5 retry\n");
        Serial1.println("Please check server's ip and port, also check AccessToken\n");
        digitalWrite(STATUS_LED, 0);
        break;
    case CONNECTED:
    case KEEP_ALIVE:
        os_timer_arm(&timer_network_status_indicate[0], 1000, false);
        digitalWrite(STATUS_LED, ~digitalRead(STATUS_LED));
        break;
    default:
        break;
    }

    last_main_status = conn_status[0];
}

#endif





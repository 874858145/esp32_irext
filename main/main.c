/* HTTPS GET Example using plain mbedTLS sockets
 *
 * Contacts the howsmyssl.com API via TLS v1.2 and reads a JSON
 * response.
 *
 * Adapted from the ssl_client1 example in mbedtls.
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "IRsend.h"
#include "smartconfig.h"
#include "httpsdown.h"
#include "mywifi.h"
#include "udpsocket.h"
#include "my_timer.h"
#include "my_led.h"
#include "myspiffs.h"
/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
//static const char *TAG = "esp32irext";

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    Initializing_SPIFFS();   //需要在menuconfig里面设置
    setTurnOnOffFlag(false);
    my_irext_event_group = xEventGroupCreate();
    my_tg0_timer_init(TIMER_0, TEST_WITH_RELOAD, TIMER_INTERVAL0_SEC);
    my_tg0_timer_start(TIMER_0);
    initialise_wifi();
    rmt_tx_int();

	xTaskCreate(&my_timer_task, "my_timer_task", 2048, NULL, 5, &myTimerTask);
    xTaskCreate(&my_led_task, "my_led_task", 4096, NULL, 5, &ledControlTask);
    xTaskCreate(&https_post_task, "https_post", 8192, NULL, 5, &httpDownTask);
    xTaskCreate(udp_sendip_task, "udp_sendip", 4096, NULL, 4, &udpSendIpTask);
    xTaskCreate(udp_receive_task, "udp_server", 4096, NULL, 5, &udpReceiveTask);
}

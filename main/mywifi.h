/*
 * mywifi.h
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */

#ifndef MAIN_MYWIFI_H_
#define MAIN_MYWIFI_H_
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"

#include "smartconfig.h"
#include "udpsocket.h"
#include "myspiffs.h"

void setSSidAndPassword(char *ssidAndPassword);
void initialise_wifi();

extern const int CONNECTED_BIT;

extern char myIpAddress[20];

extern EventGroupHandle_t my_irext_event_group;

#endif /* MAIN_MYWIFI_H_ */

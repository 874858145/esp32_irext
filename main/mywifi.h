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

void initialise_wifi(void);

extern const int IPV4_GOTIP_BIT;
extern const int IPV6_GOTIP_BIT;
extern const int ESPTOUCH_DONE_BIT;

extern EventGroupHandle_t my_irext_event_group;

#endif /* MAIN_MYWIFI_H_ */

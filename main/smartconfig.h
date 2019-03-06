/*
 * smartconfig.h
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */

#ifndef MAIN_SMARTCONFIG_H_
#define MAIN_SMARTCONFIG_H_
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_smartconfig.h"
#include "tcpip_adapter.h"

#include "mywifi.h"
#include "my_led.h"

void my_smartconfig_start();

extern TaskHandle_t smartConfigTask;

#endif /* MAIN_SMARTCONFIG_H_ */

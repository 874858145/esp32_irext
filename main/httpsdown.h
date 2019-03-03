/*
 * httpsdown.h
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */

#ifndef MAIN_HTTPSDOWN_H_
#define MAIN_HTTPSDOWN_H_
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_tls.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"

#include "cJSON.h"
#include "ir_decode.h"
#include "IRsend.h"
#include "smartconfig.h"

void Initializing_SPIFFS(void);
void https_get_task(void *pvParameters);

#endif /* MAIN_HTTPSDOWN_H_ */

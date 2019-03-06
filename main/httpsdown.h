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


#include "cJSON.h"
#include "ir_decode.h"
#include "IRsend.h"
#include "smartconfig.h"
#include "myspiffs.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "www.irext.net"
#define WEB_PORT "443"
#define WEB_URL "https://irext.net/irext-server/operation/download_bin"

#define CONTENTTYPE "application/json"

void https_post_task(void *pvParameters);

extern TaskHandle_t httpDownTask;
extern t_remote_ac_status ac_status;

#endif /* MAIN_HTTPSDOWN_H_ */

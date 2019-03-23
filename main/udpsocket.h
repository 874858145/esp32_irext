/*
 * udpsocket.h
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */

#ifndef MAIN_UDPSOCKET_H_
#define MAIN_UDPSOCKET_H_
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "cJSON.h"
#include "ir_decode.h"
#include "IRsend.h"
#include "mywifi.h"
#include "myspiffs.h"

typedef struct
{
	int indexId;
	int categoryId;

}MY_CONTROL_t;

typedef struct
{
	int power;
	int temperature;
	int mode;
	int wind_Speed;
	int wind_Swing;
}MY_AC_CONTROL_t;

void udp_sendip_task(void *pvParameters);
void udp_receive_task(void *pvParameters);

extern MY_AC_CONTROL_t my_ac_control;

extern TaskHandle_t udpSendIpTask;
extern TaskHandle_t udpReceiveTask;

#endif /* MAIN_UDPSOCKET_H_ */

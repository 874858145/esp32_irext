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

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "cJSON.h"
#include "ir_decode.h"
#include "mywifi.h"
#include "httpsdown.h"

typedef struct
{
	char id[5];
	char token[40];
	char indexId[10];
	char categoryId[5];
	char power[5];
	char temperature[5];
	char mode[5];
	char wind_Speed[5];
	char wind_Swing[5];
}MY_AC_CONTROL_t;

void udp_sendip_task(void *pvParameters);
void udp_receive_task(void *pvParameters);

extern MY_AC_CONTROL_t my_ac_control;

extern TaskHandle_t udpSendIpTask;
extern TaskHandle_t udpReceiveTask;

extern const int GETACCONTROL_BIT;

#endif /* MAIN_UDPSOCKET_H_ */

/*
 * udpsocket.h
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */

#ifndef MAIN_UDPSOCKET_H_
#define MAIN_UDPSOCKET_H_
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "mywifi.h"

#define PORT 9696

void udp_server_task(void *pvParameters);

#endif /* MAIN_UDPSOCKET_H_ */

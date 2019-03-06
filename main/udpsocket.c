/*
 * udpsocket.c
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */
#include "udpsocket.h"

static const char *TAG = "udpsocket";

MY_AC_CONTROL_t my_ac_control;

TaskHandle_t udpSendIpTask;
TaskHandle_t udpReceiveTask;

const int GETACCONTROL_BIT = BIT1;

static void wait_for_ip()
{
    ESP_LOGI(TAG, "Waiting for AP connection...");
    xEventGroupWaitBits(my_irext_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");
}

void setACControlDate(cJSON * root)
{
	cJSON *tempJson = NULL;
	char *tempStr = NULL;

	tempJson = cJSON_GetObjectItem(root, "id");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.id,tempStr);

	tempJson = cJSON_GetObjectItem(root, "token");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.token,tempStr);

	tempJson = cJSON_GetObjectItem(root, "indexId");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.indexId,tempStr);

	tempJson = cJSON_GetObjectItem(root, "categoryId");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.categoryId,tempStr);

	tempJson = cJSON_GetObjectItem(root, "power");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.power,tempStr);

	tempJson = cJSON_GetObjectItem(root, "temperature");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.temperature,tempStr);

	tempJson = cJSON_GetObjectItem(root, "mode");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.mode,tempStr);

	tempJson = cJSON_GetObjectItem(root, "wind_Speed");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.wind_Speed,tempStr);

	tempJson = cJSON_GetObjectItem(root, "wind_Swing");
	tempStr = cJSON_Print(tempJson);
	strcpy(my_ac_control.wind_Swing,tempStr);
}

void udp_sendip_task(void *pvParameters)   //如果ip为空就发送NULL,否则发送ip
{
	char tx_buffer[128];
	char addr_str[128];
	int addr_family;
	int ip_protocol;

	cJSON *jsonroot=0;
	char *jsonStr = 0;

	while (1) {
		wait_for_ip();

		struct sockaddr_in destAddr;
		destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		destAddr.sin_family = AF_INET;
		destAddr.sin_port = htons(PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;
		inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

		int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
		if (sock < 0) {
			ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
			break;
		}
		ESP_LOGI(TAG, "Socket created");
		while (1) {
			jsonroot = cJSON_CreateObject();
			cJSON_AddStringToObject(jsonroot, "ipAddress", myIpAddress);
			jsonStr = cJSON_Print(jsonroot);
			if(jsonStr)
				strcpy(tx_buffer,jsonStr);
			else
				strcpy(tx_buffer,"NULL");
			cJSON_Delete(jsonroot);

			int err = sendto(sock, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
			if (err < 0) {
				ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
				break;
			}
			ESP_LOGI(TAG, "Message sent");

			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}
		if (sock != -1) {
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			shutdown(sock, 0);
			close(sock);
		}
	}
	vTaskDelete(udpSendIpTask);
}

void udp_receive_task(void *pvParameters)
{
    char rx_buffer[256];
    char addr_str[256];
    int addr_family;
    int ip_protocol;

    cJSON * acjson = NULL;

    while (1) {
    	wait_for_ip();

        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket binded");

        while (1) {

            ESP_LOGI(TAG, "Waiting for data");
            struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);

                if(strlen(rx_buffer)>50)   //表示接收到控制指令
                {
                	acjson = cJSON_Parse(rx_buffer);
					if(acjson)
					{
						setACControlDate(acjson);
						ESP_LOGI(TAG, "id:%s", my_ac_control.id);
						ESP_LOGI(TAG, "token:%s", my_ac_control.token);
						ESP_LOGI(TAG, "indexId:%s", my_ac_control.indexId);
						ESP_LOGI(TAG, "categoryId:%s", my_ac_control.categoryId);
						ESP_LOGI(TAG, "power:%s", my_ac_control.power);
						ESP_LOGI(TAG, "temperature:%s", my_ac_control.temperature);
						ESP_LOGI(TAG, "mode:%s", my_ac_control.mode);
						ESP_LOGI(TAG, "wind_Speed:%s", my_ac_control.wind_Speed);
						ESP_LOGI(TAG, "wind_Swing:%s", my_ac_control.wind_Swing);

						switch(atoi(my_ac_control.power))   //设置开关
						{
						case 0:
							ac_status.ac_power = AC_POWER_ON;
							break;
						case 1:
							ac_status.ac_power = AC_POWER_OFF;
							break;
						default:
							ac_status.ac_power = AC_POWER_ON;
							break;
						}

						ac_status.ac_temp = atoi(my_ac_control.temperature)-16;   //设置温度

						switch(atoi(my_ac_control.mode))   //设置模式
						{
						case 0:
							ac_status.ac_mode = AC_MODE_COOL;
							break;
						case 1:
							ac_status.ac_mode = AC_MODE_HEAT;
							break;
						case 2:
							ac_status.ac_mode = AC_MODE_AUTO;
							break;
						case 3:
							ac_status.ac_mode = AC_MODE_FAN;
							break;
						case 4:
							ac_status.ac_mode = AC_MODE_DRY;
							break;
						default:
							ac_status.ac_mode = AC_MODE_COOL;
							break;
						}

						switch(atoi(my_ac_control.wind_Speed))   //设置模式
						{
						case 0:
							ac_status.ac_wind_dir = AC_WS_AUTO;
							break;
						case 1:
							ac_status.ac_wind_dir = AC_WS_LOW;
							break;
						case 2:
							ac_status.ac_wind_dir = AC_WS_MEDIUM;
							break;
						case 3:
							ac_status.ac_wind_dir = AC_WS_HIGH;
							break;
						default:
							ac_status.ac_wind_dir = AC_WS_AUTO;
							break;
						}

						switch(atoi(my_ac_control.wind_Speed))   //设置模式
						{
						case 0:
							ac_status.ac_wind_speed = AC_SWING_ON;
							break;
						case 1:
							ac_status.ac_wind_speed = AC_SWING_OFF;
							break;
						default:
							ac_status.ac_wind_speed = AC_SWING_ON;
							break;
						}

						xEventGroupSetBits(my_irext_event_group, GETACCONTROL_BIT);
					}
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(udpReceiveTask);
}


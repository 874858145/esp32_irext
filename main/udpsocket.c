/*
 * udpsocket.c
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */
#include "udpsocket.h"

static const char *TAG = "udpsocket";

t_remote_ac_status ac_status =
{
	// 默认空调状态
	AC_POWER_ON,
	AC_TEMP_24,
	AC_MODE_COOL,
	AC_SWING_ON,
	AC_WS_AUTO,
	1,
	0,
	0
};

TaskHandle_t udpSendIpTask;
TaskHandle_t udpReceiveTask;

unsigned short user_data[USER_DATA_SIZE];

const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void wait_for_ip()
{
    ESP_LOGI(TAG, "Waiting for AP connection...");
    xEventGroupWaitBits(my_irext_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");
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
		destAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
		destAddr.sin_family = AF_INET;
		destAddr.sin_port = htons(9696);
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
			//ESP_LOGI(TAG,"send:%s,%d" ,jsonStr,err);

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

void ACStatusCinfig(const MY_AC_CONTROL_t my_ac_control)
{
	switch(my_ac_control.power)   //设置开关
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

	ac_status.ac_temp = my_ac_control.temperature-16;   //设置温度

	switch(my_ac_control.mode)   //设置模式
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

	switch(my_ac_control.wind_Speed)   //设置模式
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

	switch(my_ac_control.wind_Speed)   //设置模式
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
}

/** 在字符串中查询特定字符位置索引
* const char *str ，字符串
* char c，要查找的字符
*/
inline int num_strchr(const char *str, char c)
{
	const char *pindex = strchr(str, c);
	if (NULL == pindex){
		return -1;
	}
	return pindex - str;
}

/* 解码
* const char * base64 码字
* unsigned char * dedata， 解码恢复的数据
*/
int base64_decode(const char * base64, unsigned char * dedata)
{
	int i = 0, j=0;
	int trans[4] = {0,0,0,0};
	for (;base64[i]!='\0';i+=4)
	{
		// 每四个一组，译码成三个字符
		trans[0] = num_strchr(base64char, base64[i]);
		trans[1] = num_strchr(base64char, base64[i+1]);
		// 1/3
		dedata[j++] = ((trans[0] << 2) & 0xfc) | ((trans[1]>>4) & 0x03);
		if (base64[i+2] == '=')
		{
			continue;
		} else{
			trans[2] = num_strchr(base64char, base64[i + 2]);
		}
		// 2/3
		dedata[j++] = ((trans[1] << 4) & 0xf0) | ((trans[2] >> 2) & 0x0f);
		if (base64[i + 3] == '=')
		{
			continue;
		}
		else{
			trans[3] = num_strchr(base64char, base64[i + 3]);
		}
		// 3/3
		dedata[j++] = ((trans[2] << 6) & 0xc0) | (trans[3] & 0x3f);
	}
	dedata[j] = '\0';
	return 0;
}

void getControlDate(cJSON * root)
{
	int ret = 0;

	cJSON * controlJson = NULL;
	cJSON * fileJson = NULL;
	int fileFlag = 0;

	char pathName[10] = "/irext/";
	char filePath[20];

	MY_CONTROL_t my_control;
	MY_AC_CONTROL_t my_ac_control;

//	tempJson = cJSON_GetObjectItem(root, "wind_Swing");
//	tempStr = cJSON_Print(tempJson);
//	strcpy(my_ac_control.wind_Swing,tempStr);
	//接收json数据
	my_control.indexId = cJSON_GetObjectItem(root, "indexId")->valueint;
	my_control.categoryId = cJSON_GetObjectItem(root, "categoryId")->valueint;
	controlJson = cJSON_GetObjectItem(root, "control");
	my_ac_control.power = cJSON_GetObjectItem(controlJson, "power")->valueint;
	my_ac_control.temperature = cJSON_GetObjectItem(controlJson, "temperature")->valueint;
	my_ac_control.mode = cJSON_GetObjectItem(controlJson, "mode")->valueint;
	my_ac_control.wind_Speed = cJSON_GetObjectItem(controlJson, "wind_Speed")->valueint;
	my_ac_control.wind_Swing = cJSON_GetObjectItem(controlJson, "wind_Swing")->valueint;

	ESP_LOGI(TAG, "indexId:%d", my_control.indexId);
	ESP_LOGI(TAG, "categoryId:%d", my_control.categoryId);
	ESP_LOGI(TAG, "power:%d", my_ac_control.power);
	ESP_LOGI(TAG, "temperature:%d", my_ac_control.temperature);
	ESP_LOGI(TAG, "mode:%d", my_ac_control.mode);
	ESP_LOGI(TAG, "wind_Speed:%d", my_ac_control.wind_Speed);
	ESP_LOGI(TAG, "wind_Swing:%d", my_ac_control.wind_Swing);

	//配置空调参数
	ACStatusCinfig(my_ac_control);

	sprintf(filePath,"%s%d",pathName,my_control.indexId);
	ESP_LOGI(TAG, "filePath:%s", filePath);
	FILE* f = fopen(filePath, "r");
	if(f==NULL)
	{
		fileFlag = cJSON_GetObjectItem(root, "fileFlag")->valueint;
		if(fileFlag)
		{
			fclose(f);

			fileJson = cJSON_GetObjectItem(root, "file");
			unsigned short blen = cJSON_GetObjectItem(fileJson, "blen")->valueint;
			unsigned short flen = cJSON_GetObjectItem(fileJson, "flen")->valueint;
			char *base64Str = (char*)calloc((blen+1), sizeof(char));
			//加一是为了去掉开头的双引号
			memcpy(base64Str,(cJSON_Print(cJSON_GetObjectItem(fileJson, "binaryFile"))+1),blen);
			unsigned char *inputFile= (unsigned char *)calloc((flen+1), sizeof(unsigned char));
			base64_decode(base64Str,inputFile);
			free(base64Str);

			f = fopen(filePath, "w");
			fwrite(inputFile,flen,1,f);
			fclose(f);

			//红外解码
			ret = ir_binary_open(IR_CATEGORY_AC, 1, inputFile, flen);
			if(ret!=-1)
			{
				int length = ir_decode(2, user_data, &ac_status, 0);
//				for (int i = 0; i < length; i++) {
//					printf("%d ", user_data[i]);
//				}
//				printf("\r\n");

				IRsend(user_data,length);
			}
			free(inputFile);
		}
		else
		{
			ESP_LOGI(TAG, "don't have file");
		}
	}
	else
	{
		fseek(f,0L,SEEK_END);
		unsigned short content_length=ftell(f);
		if(content_length>0)
		{
			unsigned char *content= (unsigned char *)malloc(content_length * sizeof(unsigned char));
			fseek(f,0L,SEEK_SET);
			fread((char*)content,content_length,1,f);
			ESP_LOGI(TAG, "content_length:%d", content_length);
			ESP_LOGI(TAG, "content:%s", content);
			ret = ir_binary_open(IR_CATEGORY_AC, 1, content, content_length);
			if(ret!=-1)
			{
				int length = ir_decode(2, user_data, &ac_status, 0);
//				for (int i = 0; i < length; i++) {
//					printf("%d ", user_data[i]);
//				}
//				printf("\r\n");

				IRsend(user_data,length);
			}
			free(content);
			fclose(f);
		}
		else
		{
			fclose(f);
			ESP_LOGI(TAG, "open irext error!!");
		}
	}
}

void udp_receive_task(void *pvParameters)
{
    char rx_buffer[1536];
    char addr_str[256];
    int addr_family;
    int ip_protocol;

    cJSON * acjson = NULL;

    while (1) {
    	wait_for_ip();

        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(6969);
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
						getControlDate(acjson);
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


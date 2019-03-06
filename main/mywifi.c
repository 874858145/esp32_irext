/*
 * mywifi.c
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */
#include "mywifi.h"

static const char *TAG = "my_wifi";

const int CONNECTED_BIT = BIT0;

char myIpAddress[20];

EventGroupHandle_t my_irext_event_group;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
    	esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
    	ESP_LOGI(TAG, "got ip:%s",ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    	strcpy(myIpAddress,ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    	xEventGroupSetBits(my_irext_event_group, CONNECTED_BIT);
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
		break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(my_irext_event_group, CONNECTED_BIT);
		break;
    default:
        break;
    }
    return ESP_OK;
}

void setSSidAndPassword(char *ssidAndPassword)
{
	FILE* f = fopen("/irext/wifi", "w");
	if(f!=NULL)
	{
		fwrite(ssidAndPassword,strlen(ssidAndPassword),1,f);
	}
	fclose(f);
}

void getSSidAndPassword(char *ssidAndPassword)
{
	char content[64];
	FILE* f = fopen("/irext/wifi", "r");
	if(f!=NULL)
	{
		fclose(f);
		FILE* f = fopen("/irext/wifi", "w");
		if(f!=NULL)
		{
			memset(content,0,64);
			fseek(f,0L,SEEK_END);
			unsigned short content_length=ftell(f);
			fseek(f,0L,SEEK_SET);
			fread(content,content_length,1,f);
			strcpy(ssidAndPassword,content);
		}
		fclose(f);
	}
	else
	{
		strcpy(ssidAndPassword,"");
	}
}

void initialise_wifi()
{
	char ssidAndPassword[64];
	char ssid[32];
	char password[32];
	char *p;
	int i=0;

    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );   //第二个参数为回调要传的参数

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    memset(ssidAndPassword,0,64);
    getSSidAndPassword(ssidAndPassword);
    if(strcmp(ssidAndPassword,"")==0)
    {
    	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
		ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
		ESP_ERROR_CHECK( esp_wifi_start() );
    }
    else
    {
    	memset(ssid,0,32);
    	memset(password,0,32);
    	p = strtok(ssidAndPassword, ",");
    	while(p!=NULL)
    	{
    		p = strtok(ssidAndPassword, ",");
    		if(i==0)
    			strcpy(ssid,p);
    		else if(i==1)
    		{
    			strcpy(password,p);
    		}
    		i++;
    	}
    	ESP_LOGI(TAG, "ssid:%s",ssid);
    	ESP_LOGI(TAG, "password:%s",password);

    	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM));
    	wifi_config_t wifi_config;
    	strcpy((char *)wifi_config.sta.ssid,ssid);
    	strcpy((char *)wifi_config.sta.password,password);

		ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
		ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
		ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
		ESP_ERROR_CHECK( esp_wifi_start() );
    }
}

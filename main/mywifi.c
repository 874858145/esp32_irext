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
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
//    	ESP_LOGI(TAG, "got ip:%s",ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    	strcpy(myIpAddress,ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    	ESP_LOGI(TAG, "got ip:%s",myIpAddress);
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

void initialise_wifi(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}


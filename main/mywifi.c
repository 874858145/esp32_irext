/*
 * mywifi.c
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */
#include "mywifi.h"

static const char *TAG = "my_wifi";

const int IPV4_GOTIP_BIT = BIT0;
const int IPV6_GOTIP_BIT = BIT1;
const int ESPTOUCH_DONE_BIT = BIT2;

EventGroupHandle_t my_irext_event_group;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, &smartConfigTask);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
		/* enable ipv6 */
		tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
		break;
    case SYSTEM_EVENT_STA_GOT_IP:
    	xEventGroupSetBits(my_irext_event_group, IPV4_GOTIP_BIT);
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
		break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(my_irext_event_group, IPV4_GOTIP_BIT);
		xEventGroupClearBits(my_irext_event_group, IPV6_GOTIP_BIT);
		break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
		xEventGroupSetBits(my_irext_event_group, IPV6_GOTIP_BIT);
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6");
		char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
		ESP_LOGI(TAG, "IPv6: %s", ip6);
    default:
        break;
    }
    return ESP_OK;
}

void initialise_wifi(void)
{
    tcpip_adapter_init();
    my_irext_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );   //第二个参数为回调要传的参数

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

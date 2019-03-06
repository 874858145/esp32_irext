/*
 * smartconfig.c
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */
#include "smartconfig.h"

static const char *TAG = "my_smartconfig";

TaskHandle_t smartConfigTask;

static void sc_callback(smartconfig_status_t status, void *pdata)
{
	char ssidAndPassword[64];

    switch (status) {
        case SC_STATUS_WAIT:
            ESP_LOGI(TAG, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            ESP_LOGI(TAG, "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;
            ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
            ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);

            memset(ssidAndPassword,0,64);    //保存ssid和密码
            strcpy (ssidAndPassword,(char *)wifi_config->sta.ssid);
            strcat(ssidAndPassword,",");
            strcat(ssidAndPassword,(char *)wifi_config->sta.password);
            setSSidAndPassword(ssidAndPassword);

            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config) );
            ESP_ERROR_CHECK( esp_wifi_connect() );
            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                uint8_t phone_ip[4] = { 0 };
                memcpy(phone_ip, (uint8_t* )pdata, 4);
                ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            xEventGroupClearBits(my_irext_event_group, LEDCONTROL_BIT);
            esp_smartconfig_stop();
            ESP_LOGI(TAG, "smartconfig over");
            break;
        default:
            break;
    }
}

void my_smartconfig_start()
{
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );
}

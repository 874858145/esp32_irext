/* HTTPS GET Example using plain mbedTLS sockets
 *
 * Contacts the howsmyssl.com API via TLS v1.2 and reads a JSON
 * response.
 *
 * Adapted from the ssl_client1 example in mbedtls.
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "ir_decode.h"
#include "IRsend.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "www.irext.net"
#define WEB_PORT "443"
#define WEB_URL "https://irext.net/irext-server/operation/download_bin"

#define CONTENTTYPE "application/json"

static const char *TAG = "Https_example";

static t_remote_ac_status ac_status =
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

unsigned short user_data[USER_DATA_SIZE];

/* Root cert for howsmyssl.com, taken from server_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");
    
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void Initializing_SPIFFS(void)
{
	esp_vfs_spiffs_conf_t conf = {
	  .base_path = "/irext",
	  .partition_label = NULL,
	  .max_files = 5,
	  .format_if_mount_failed = true
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}
}

/*将大写字母转换成小写字母*/
int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 'a' - 'A';
    }
    else
    {
        return c;
    }
}

//16进制字符串转int
int hextoint(char s[])
{
    int i;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)
    {
        if (tolower(s[i]) > '9')
        {
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (tolower(s[i]) - '0');
        }
    }
    return n;
}

static void https_get_task(void *pvParameters)
{
    int ret;

    cJSON *jsonroot=0;
    char *writeJson=0;
    char writeJsonLen[10];

//    char *irextFileName=0;
    char request_buf[2048];

    char id[] = "52";
    char token[] = "34b3523d6b2ee5b12e9ba981546fff78";
    char indexId[] = "3521";

    jsonroot = cJSON_CreateObject();
	cJSON_AddStringToObject(jsonroot, "id", id);
	cJSON_AddStringToObject(jsonroot, "token", token);
	cJSON_AddStringToObject(jsonroot, "indexId", indexId);
	writeJson = cJSON_Print(jsonroot);
	sprintf(writeJsonLen, "%d", strlen(writeJson));

	cJSON_Delete(jsonroot);

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");
        esp_tls_cfg_t cfg = {
            .cacert_pem_buf  = server_root_cert_pem_start,
            .cacert_pem_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
        };

        //struct esp_tls *tls = esp_tls_conn_http_new(WEB_URL, &cfg);
        struct esp_tls *tls = esp_tls_conn_http_new(WEB_URL, &cfg);
        
        if(tls != NULL) {
            ESP_LOGI(TAG, "Connection established...");
        } else {
            ESP_LOGE(TAG, "Connection failed...");
            goto exit;
        }

        memset(request_buf, 0, 2048);
        strcat(request_buf, "POST " WEB_URL " HTTP/1.1\r\n");
		strcat(request_buf, "Host: "WEB_SERVER"\r\n");
		strcat(request_buf, "Content-Type: "CONTENTTYPE"\r\n");
		strcat(request_buf, "Content-Length: ");
		strcat(request_buf, writeJsonLen);
		strcat(request_buf, "\r\n\r\n");
		strcat(request_buf, writeJson);
		strcat(request_buf, "\r\n");

		for(int i = 0; i < strlen(request_buf); i++) {
			putchar(request_buf[i]);
		}
        size_t written_bytes = 0;
        do {
            ret = esp_tls_conn_write(tls, 
            						request_buf + written_bytes,
									strlen(request_buf) - written_bytes);
            if (ret >= 0) {
                ESP_LOGI(TAG, "%d bytes written", ret);
                written_bytes += ret;
            } else if (ret != MBEDTLS_ERR_SSL_WANT_READ  && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
               ESP_LOGE(TAG, "esp_tls_conn_write  returned 0x%x", ret);
                goto exit;
            }
        } while(written_bytes < strlen(request_buf));

        ESP_LOGI(TAG, "Reading HTTP response...");

//        memset(irextFileName, 0, sizeof("/irext/"indexId));
//        strcat(irextFileName, "/irext/"indexId);

//        FILE* f = fopen(irextFileName, "w");

        FILE* f = fopen("/irext/2", "w");
        do
        {
    		const int bufsize = 1024*2;
    		char* buf = (char*)calloc(bufsize, 1);
            ret = esp_tls_conn_read(tls, (char *)buf, bufsize-1);   //读取设置大小的数据

            if(ret == MBEDTLS_ERR_SSL_WANT_WRITE  || ret == MBEDTLS_ERR_SSL_WANT_READ)
                continue;
            
            if(ret < 0)
           {
                ESP_LOGE(TAG, "esp_tls_conn_read  returned -0x%x", -ret);
                break;
           }

            if(ret == 0)
            {
                ESP_LOGI(TAG, "connection closed");
                break;
            }

            if(ret > 0)
            {
            	char inputFile[1024];
            	char *tempBuf = strstr(buf, "\r\n\r\n")+4;
            	char cbinLen[10];
            	memcpy(cbinLen, tempBuf, 3);
            	cbinLen[3]='\0';
            	int binLen = hextoint(cbinLen);
            	memcpy(inputFile, buf+ret-(binLen+7), binLen);
//            	for(int i = 0; i < binLen; i++) {
//					putchar(inputFile[i]);
//				}
            	for(int i = 0; i < ret; i++) {
					putchar(buf[i]);
				}

            	fwrite(inputFile,binLen,1,f);
            }
            free(buf);
            fclose(f);

//            len = ret;
//            ESP_LOGD(TAG, "%d bytes read", len);

            f = fopen("/irext/2", "r");
			if (f == NULL) {
				ESP_LOGE(TAG, "Failed to open file for reading");
			}
			else{
//				char line[64];
//				fgets(line, sizeof(line), f);

			    fseek(f,0L,SEEK_END);
			    unsigned short content_length=ftell(f);
			    unsigned char *content= (unsigned char *)malloc(content_length * sizeof(unsigned char));
			    fseek(f,0L,SEEK_SET);
			    fread((char*)content,content_length,1,f);
			    ret = ir_binary_open(IR_CATEGORY_AC, 1, content, content_length);
			    int length = ir_decode(2, user_data, &ac_status, 0);
			    for (int i = 0; i < length; i++) {
					printf("%d ", user_data[i]);
				}
			    printf("\r\n");

//			    IRsend(user_data,length);

			    fclose(f);
//			    printf("size:%d\r\n",content_length);
//			    printf("3:%s\r\n",line);
				//goto exit;
			    while(1)
			    {
			    	IRsend(user_data,length);
			    	vTaskDelay(500 / portTICK_PERIOD_MS);
			    }
			}
        } while(1);

    exit:
        esp_tls_conn_delete(tls);
        putchar('\n'); // JSON output doesn't have a newline at end

        static int request_count;
        ESP_LOGI(TAG, "Completed %d requests", ++request_count);

        for(int countdown = 2; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    Initializing_SPIFFS();   //需要在menuconfig里面设置
    rmt_tx_int();

    xTaskCreate(&https_get_task, "https_get_task", 8192, NULL, 5, NULL);
}

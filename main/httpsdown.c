/*
 * httpsdown.c
 *
 *  Created on: 2019-3-2
 *      Author: ubuntu
 */
#include "httpsdown.h"

static const char *TAG = "my_httpsdown";

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

unsigned short user_data[USER_DATA_SIZE];

TaskHandle_t httpDownTask;

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

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

void https_post_task(void *pvParameters)
{
    int ret;

    struct esp_tls *tls = NULL;

    char *pathName = "/irext/";
    char *filePath=0;

    cJSON *jsonroot=0;
    char *writeJson=0;
    char writeJsonLen[10];

    char request_buf[2048];

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        xEventGroupWaitBits(my_irext_event_group, GETACCONTROL_BIT,false, true, portMAX_DELAY);
        xEventGroupClearBits(my_irext_event_group, GETACCONTROL_BIT);

        sprintf(filePath,"%s%s",pathName,my_ac_control.indexId);
        ESP_LOGI(TAG, "filePath:%s", filePath);
        FILE* f = fopen(filePath, "r");
        if(f==NULL)
        {
        	fclose(f);
        	jsonroot = cJSON_CreateObject();
			cJSON_AddStringToObject(jsonroot, "id", my_ac_control.id);
			cJSON_AddStringToObject(jsonroot, "token", my_ac_control.token);
			cJSON_AddStringToObject(jsonroot, "indexId", my_ac_control.indexId);
			writeJson = cJSON_Print(jsonroot);
			sprintf(writeJsonLen, "%d", strlen(writeJson));

			esp_tls_cfg_t cfg = {
				.cacert_pem_buf  = server_root_cert_pem_start,
				.cacert_pem_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
			};

			tls = esp_tls_conn_http_new(WEB_URL, &cfg);

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
			do {TaskHandle_t httpDownTask;
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

			f = fopen(filePath, "w");

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
					for(int i = 0; i < ret; i++) {
						putchar(buf[i]);
					}

					fwrite(inputFile,binLen,1,f);
				}
				free(buf);
				fclose(f);

				f = fopen(filePath, "r");
				if (f == NULL) {
					ESP_LOGE(TAG, "Failed to open file for reading");
				}
				else{
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
					free(content);
					fclose(f);

					IRsend(user_data,length);
//					while(1)
//					{
//						IRsend(user_data,length);
//						vTaskDelay(500 / portTICK_PERIOD_MS);
//					}
				}
			} while(1);
        }
        else
        {
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
			fclose(f);

			IRsend(user_data,length);
//					while(1)
//					{
//						IRsend(user_data,length);
//						vTaskDelay(500 / portTICK_PERIOD_MS);
//					}
        }

    exit:
        esp_tls_conn_delete(tls);
        putchar('\n'); // JSON output doesn't have a newline at end
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Starting again!");
    }
}


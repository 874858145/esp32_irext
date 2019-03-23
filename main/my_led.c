/*
 * led.c
 *
 *  Created on: 2019-3-5
 *      Author: ubuntu
 */

#include "my_led.h"

static const char *TAG = "myled";

const int LEDCONTROL_BIT = BIT2;

TaskHandle_t ledControlTask;

void my_led_task(void *pvParameter)
{
    //选择芯片管脚
    gpio_pad_select_gpio(LED);
    //设置该管脚为输出模式
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    while(1) {
    	xEventGroupWaitBits(my_irext_event_group, LEDCONTROL_BIT, false, true, portMAX_DELAY);
        //电平为低
        gpio_set_level(LED, 0);
        //延迟1S
        vTaskDelay(500 / portTICK_PERIOD_MS);
        //电平为高
        gpio_set_level(LED, 1);
        //延迟1S
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

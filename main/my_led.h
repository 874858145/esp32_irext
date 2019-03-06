/*
 * my_led.h
 *
 *  Created on: 2019-3-5
 *      Author: ubuntu
 */

#ifndef MAIN_MY_LED_H_
#define MAIN_MY_LED_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "mywifi.h"

#define LED 2  //板载led口为2

void my_led_task(void *pvParameter);

extern const int LEDCONTROL_BIT;
extern TaskHandle_t ledControlTask;

#endif /* MAIN_MY_LED_H_ */

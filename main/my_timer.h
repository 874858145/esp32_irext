/*
 * timer.h
 *
 *  Created on: 2019-3-5
 *      Author: ubuntu
 */

#ifndef MAIN_MY_TIMER_H_
#define MAIN_MY_TIMER_H_
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "my_led.h"
#include "myspiffs.h"
#include "mywifi.h"
#include "smartconfig.h"

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (3.00) // sample test interval for the first timer
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

void my_tg0_timer_init(int timer_idx,bool auto_reload, double timer_interval_sec);
void setTurnOnOffFlag(bool isReset);
void my_tg0_timer_start(int timer_idx);
void my_tg0_timer_stop(int timer_idx);
void my_timer_task(void *arg);

extern TaskHandle_t myTimerTask;
extern const int MYTIMER_BIT;

#endif /* MAIN_MY_TIMER_H_ */

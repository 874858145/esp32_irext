/*
 * timer.c
 *
 *  Created on: 2019-3-5
 *      Author: ubuntu
 */

#include "my_timer.h"

static const char *TAG = "mytimer";

const int MYTIMER_BIT = BIT1;

TaskHandle_t myTimerTask;

void IRAM_ATTR my_timer_group0_isr(void *para)
{
    int timer_idx = (int) para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    //TIMERG0.hw_timer[timer_idx].update = 1;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
    	TIMERG0.int_clr_timers.t0 = 1;

    	xEventGroupSetBits(my_irext_event_group, MYTIMER_BIT);
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;
}

void my_tg0_timer_init(int timer_idx,
    bool auto_reload, double timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, my_timer_group0_isr,
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);
}

void my_tg0_timer_start(int timer_idx)  //开启定时器
{
	timer_start(TIMER_GROUP_0, timer_idx);
}

void my_tg0_timer_stop(int timer_idx)  //暂停定时器
{
	timer_pause(TIMER_GROUP_0,timer_idx);
}

void setTurnOnOffFlag(bool isReset)   //形参：true设置标志位为0，false设置在原来的数值加一。
{
	int flag=0;

	FILE* f = fopen("/irext/flag", "r");
	if(f==NULL)
	{
		fclose(f);
		f = fopen("/irext/flag", "w");
		if(f!=NULL)
		{
			if (!isReset)
				flag=1;
			else
				flag=0;
			fwrite(&flag,sizeof(int),1,f);
		}
		fclose(f);
	}
	else
	{
		if (!isReset)
		{
			fread(&flag,sizeof(int),1,f);
			fclose(f);
			f = fopen("/irext/flag", "w");
			if(f!=NULL)
			{
				flag = flag+1;
				fwrite(&flag,sizeof(int),1,f);
			}
			fclose(f);
		}
		else
		{
			fclose(f);
			f = fopen("/irext/flag", "w");
			if(f!=NULL)
			{
				flag = 0;
				fwrite(&flag,sizeof(int),1,f);
			}
			fclose(f);
		}
	}
	ESP_LOGI(TAG, "setflag:%d",flag);
}

//不要用fscanf和fprintf，贼坑
int getTurnOnOffFlag()
{
	int flag=0;

	FILE* f = fopen("/irext/flag", "r");
	if(f==NULL)
	{
		fclose(f);
		flag = 0;
	}
	else
	{
		fread(&flag,sizeof(int),1,f);
		fclose(f);
	}
	ESP_LOGI(TAG, "getflag:%d",flag);
	return flag;
}

void my_timer_task(void *arg)
{
	static unsigned char statusFlag = 0;
	int flag=0;

    while (1) {
    	xEventGroupWaitBits(my_irext_event_group, MYTIMER_BIT,false, true, portMAX_DELAY);
    	xEventGroupClearBits(my_irext_event_group, MYTIMER_BIT);
    	statusFlag++;

    	if (statusFlag == 1) {
			flag = getTurnOnOffFlag();
			if(flag>4)
			{
				xEventGroupSetBits(my_irext_event_group, LEDCONTROL_BIT);
				setTurnOnOffFlag(true);
				my_smartconfig_start();
				my_tg0_timer_stop(TIMER_0);  //暂停定时器
				vTaskDelete(myTimerTask);
			}
    	}
    	if (statusFlag > 1) {
    		esp_wifi_connect();
			setTurnOnOffFlag(true);
			my_tg0_timer_stop(TIMER_0);  //暂停定时器
			vTaskDelete(myTimerTask);
		}
    }
}

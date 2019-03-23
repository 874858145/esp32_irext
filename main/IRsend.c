#include "IRsend.h"

static const char* NEC_TAG = "NEC";

#define RMT_CLK_DIV      90    /*!< RMT counter clock divider */
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) */

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_TX_GPIO 18

static inline void nec_fill_item_level(rmt_item32_t* item, unsigned short high_us, unsigned short low_us)
{
    item->level0 = 1;
    item->duration0 = (high_us) / 10 * RMT_TICK_10_US;
    item->level1 = 0;
    item->duration1 = (low_us) / 10 * RMT_TICK_10_US;

    //printf("(%d,%d,%d,%d) ",item->level0,item->duration0,item->level1,item->duration1);
}

static void u16_to_rmt(const void* src, rmt_item32_t* dest, int src_size)
{
    int size = 0;
    unsigned short  *psrc = (unsigned short  *)src;

    rmt_item32_t* pdest = dest;
    while (size < src_size) {
    	if(size+2>src_size)
    	{
    		nec_fill_item_level(pdest,(*psrc),0);
    	}
    	else
    	{
    		nec_fill_item_level(pdest,(*psrc),(*(psrc+1)));
    	}
		pdest++;
        size=size+2;
        psrc=psrc+2;
    }
}

void IRsend(const void* src, size_t src_size)
{
	int channel = RMT_TX_CHANNEL;

	ESP_LOGI(NEC_TAG, "RMT TX DATA");
	int item_num = (src_size/2)+(src_size%2);   //计算转换后的大小
	size_t size = (sizeof(rmt_item32_t) * item_num);
	rmt_item32_t* item = (rmt_item32_t*) malloc(size);

	u16_to_rmt(src,item,src_size);

	rmt_write_items(channel, item, item_num, true);

	rmt_wait_tx_done(channel, portMAX_DELAY);

	free(item);
}

/*
 * Initialize the RMT Tx channel
 */
void rmt_tx_int()
{
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = RMT_TX_CHANNEL;
    config.gpio_num = RMT_TX_GPIO;
    config.mem_block_num = 1;
    config.tx_config.loop_en = 0;
    // enable the carrier to be able to hear the Morse sound
    // if the RMT_TX_GPIO is connected to a speaker
    config.tx_config.carrier_en = 1;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = 0;
    config.tx_config.carrier_duty_percent = 25;
    // set audible career frequency of 611 Hz
    // actually 611 Hz is the minimum, that can be set
    // with current implementation of the RMT API
    config.tx_config.carrier_freq_hz = 38000;
    config.tx_config.carrier_level = 1;
    // set the maximum clock divider to be able to output
    // RMT pulses in range of about one hundred milliseconds
    config.clk_div = RMT_CLK_DIV;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

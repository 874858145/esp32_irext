/*
 * IRsend.h
 *
 *  Created on: 2019-1-8
 *      Author: ubuntu
 */

#ifndef MAIN_IRSEND_H_
#define MAIN_IRSEND_H_
#include "esp_log.h"
#include "esp_spiffs.h"
#include "driver/rmt.h"
#include "driver/rmt.h"

void IRsend(const void* src, size_t src_size);
void rmt_tx_int();

#endif /* MAIN_IRSEND_H_ */

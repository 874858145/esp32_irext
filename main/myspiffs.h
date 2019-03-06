/*
 * myspiffs.h
 *
 *  Created on: 2019-3-5
 *      Author: ubuntu
 */

#ifndef MAIN_MYSPIFFS_H_
#define MAIN_MYSPIFFS_H_
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

void Initializing_SPIFFS(void);

#endif /* MAIN_MYSPIFFS_H_ */

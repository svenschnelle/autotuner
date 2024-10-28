/*
 * main.h
 *
 *  Created on: 19.09.2017
 *      Author: kurt
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <t2_led.h>
#include <t2_relais.h>
#include <setL.h>
#include <setC.h>
#include <tick1ms.h>
#include <uart.h>
#include <remote.h>
#include <crc16.h>
#include <adc.h>
#include <swr.h>
#include <tune_storage.h>
#include <uart_civ.h>
#include <eeprom.h>
#include <bandcalc.h>
#include <tuner.h>
#include <serial_tx_proc.h>

int main(void);

extern uint32_t last_freq;

#define FWVERSION "TUNER-2 V1.1"

#define ON 1
#define OFF 0
#define TOGGLE 2

#endif /* MAIN_H_ */

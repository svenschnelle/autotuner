/*
 * tick1ms.h
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

#ifndef TICK1MS_H_
#define TICK1MS_H_

void TIM3_IRQHandler(void);
void TIM3_Initialization();
void delay_1ms(int ms);

#define TIME_UNTIL_SLEEP	2000	// ms after last activity

extern volatile unsigned int civsendtick, sleep_request;
extern volatile unsigned int ee_savetimer;
extern volatile unsigned int txqrg_tick;
extern volatile uint64_t tick_1ms;
extern volatile unsigned int uart_tick;
extern volatile int got_civ_freq;

#endif /* TICK1MS_H_ */

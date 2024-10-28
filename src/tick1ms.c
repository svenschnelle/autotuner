/*
 * delay.c
 *
 *  Created on: 13.08.2017
 *      Author: kurt
 */

// Mit TIM3 wird ein 1ms Takt erzeugt, zB für die Delayfunktion

#include "main.h"

volatile unsigned int tim3_cpt = 0;
volatile unsigned int civsendtick = 0;
volatile unsigned int ee_savetimer = 0;
volatile unsigned int tunertimer = 0;
volatile unsigned int txqrg_tick = 0;
volatile unsigned int uart_tick = 0;
volatile uint64_t tick_1ms = 0;
volatile int got_civ_freq = 0;

void TIM3_Initialization()
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* Enable the TIM3 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* TIM3 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	// APB1 hat 90 MHz
	TIM_TimeBaseStructure.TIM_Period = 90;			// hier noch 1 MHz
	TIM_TimeBaseStructure.TIM_Prescaler = 1000;		// und hier 1ms
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	// Interrupt enable
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	// TIM3 einschalten
	TIM_Cmd(TIM3, ENABLE);
}

// Handle TIM3 interrupt, einmal pro ms
void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
	{
		reset_t2relais();

		if(civsendtick) civsendtick--;
		if(ee_savetimer) ee_savetimer--;
		if(uart_tick) uart_tick--;
		tim3_cpt++;
		txqrg_tick++;
		tick_1ms++;
		got_civ_freq++;
		if(tunertimer++ > tuningspeed)
		{
			tunertimer = 0;
			do_tuning = 1;
		}


		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}

void delay_1ms(int ms)
{
	if(ms == 0) return;

	tim3_cpt=0;
	while(tim3_cpt < ms);
}

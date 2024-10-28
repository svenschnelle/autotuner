/*
 * t2_led.c
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

/*
 * Bedienung der 3-farben LED auf dem Tunercontroller
 * ==================================================
 *
 * die LED ist wie folgt angeschlossen:
 * rot .... GPIOB5
 * grün ... GPIOB4
 * blaub .. GPIOB6
 *
 * Benutzung:
 * set_t2led(int color); mit einer der Farben aus t2_led.h
*/

#include <main.h>

// Initialisiere die GPIOs für Output
void init_t2led()
{
	// Schalte den Takt für GPIOB ein
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	// Schalte die GPIOs auf Output
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// setze sie auf 1, damit sind alle LEDs dunkel
	GPIO_SetBits(GPIOB, GPIO_Pin_4);
	GPIO_SetBits(GPIOB, GPIO_Pin_5);
	GPIO_SetBits(GPIOB, GPIO_Pin_6);
}

int lastcolor = 0;

void set_t2led(int color)
{
	if(color != LASTCOL)
	{
		lastcolor = color;
		set_t2col(color);
	}
	else
	{
		set_t2col(lastcolor);
	}
}

void set_t2col(int color)
{
	switch (color)
	{
	case BLACKCOL:
		GPIO_SetBits(GPIOB, GPIO_Pin_4);
		GPIO_SetBits(GPIOB, GPIO_Pin_5);
		GPIO_SetBits(GPIOB, GPIO_Pin_6);
		break;
	case GREENCOL:
		GPIO_SetBits(GPIOB, GPIO_Pin_4);
		GPIO_ResetBits(GPIOB, GPIO_Pin_5);
		GPIO_SetBits(GPIOB, GPIO_Pin_6);
		break;
	case REDCOL:
		GPIO_ResetBits(GPIOB, GPIO_Pin_4);
		GPIO_SetBits(GPIOB, GPIO_Pin_5);
		GPIO_SetBits(GPIOB, GPIO_Pin_6);
		break;
	case BLUECOL:
		GPIO_SetBits(GPIOB, GPIO_Pin_4);
		GPIO_SetBits(GPIOB, GPIO_Pin_5);
		GPIO_ResetBits(GPIOB, GPIO_Pin_6);
		break;
	case WHITECOL:
		GPIO_ResetBits(GPIOB, GPIO_Pin_4);
		GPIO_ResetBits(GPIOB, GPIO_Pin_5);
		GPIO_ResetBits(GPIOB, GPIO_Pin_6);
		break;
	}
}

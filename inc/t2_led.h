/*
 * t2_led.h
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

#ifndef T2_LED_H_
#define T2_LED_H_

void init_t2led();
void set_t2col(int color);
void set_t2led(int color);

enum LEDCOL_ {
	BLACKCOL = 0,
	REDCOL,
	GREENCOL,
	BLUECOL,
	WHITECOL,
	LASTCOL
};

#endif /* T2_LED_H_ */

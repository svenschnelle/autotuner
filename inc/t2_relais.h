/*
 * t2_relais.h
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

#ifndef T2_RELAIS_H_
#define T2_RELAIS_H_

void init_t2relais();
void reset_t2relais();
void switch_t2relais(int relais, int onoff);
int isRelaisActive();

enum {
	L_TRX = 0,
	L_ANT,
	C25,
	C50,
	C100,
	C200,
	C400,
	C800,
	C1600,
	C3200,
	L50,
	L100,
	L200,
	L400,
	L800,
	L1600,
	L3200,
	L6400,
	L12800,
	RELAISANZAHL
};

struct relay {
	GPIO_TypeDef *gpio;
	int on;
	int off;
	int state;
	int timer_on;
	int timer_off;
};

extern struct relay relays[RELAISANZAHL];

#define RELHOLDTIME 15 //20	// ms (mindestens 15 geht noch)

#endif /* T2_RELAIS_H_ */

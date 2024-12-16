/*
 * setC.h
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

#ifndef SETC_H_
#define SETC_H_

void set_C(int pF);
int calc_c();
void set_C_step(unsigned int step);
uint8_t store_actC();

extern uint8_t actCpos;

#define ANZ_C_STUFEN	256

#endif /* SETC_H_ */

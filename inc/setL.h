/*
 * setL.h
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

#ifndef SETL_H_
#define SETL_H_

void set_L(int nH);
void pos_L(int ant_trx);
int calc_l();
void set_L_step(int step);
int store_actL();
void updn_Lstep(int num);


extern int actL;
extern uint8_t actLpos;

#define ANZ_L_STUFEN	512

#endif /* SETL_H_ */

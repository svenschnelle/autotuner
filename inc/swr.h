/*
 * swr.h
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#ifndef SWR_H_
#define SWR_H_

void calc_swr(int fwd, int rev);
void init_swr();
void cal_3watt();
void cal_100watt();

extern float fwd_watt;
extern float rev_watt;
extern float fswr;

#endif /* SWR_H_ */

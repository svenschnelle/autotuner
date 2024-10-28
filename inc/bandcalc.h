/*
 * bandcalc.h
 *
 *  Created on: 31.05.2018
 *      Author: kurt
 */

#ifndef BANDCALC_H_
#define BANDCALC_H_

void getFreqEdges(uint32_t freq , uint32_t *start, uint32_t * stop);
uint8_t *get_MemUsage();
int getBandnum(uint32_t freq);

#define BANDEDGENUM 10

extern uint32_t startedges[BANDEDGENUM];
extern uint32_t stopedges[BANDEDGENUM];


#endif /* BANDCALC_H_ */

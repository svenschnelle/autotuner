/*
 * tune_storage.h
 *
 *  Created on: 28.04.2018
 *      Author: kurt
 */

#ifndef TUNE_STORAGE_H_
#define TUNE_STORAGE_H_

/*
 * 9 Spulen ... nehme 11 Bit (also 2 in Reserve)
 * 8 Kondensatoren ... nehme 11 Bit (also 3 in Reserve)
 * 2 Relais Ant/Trx ... nehme 2 Bit
 */

typedef struct
{
	uint8_t LC1;
	uint8_t LC2;
	uint8_t LC3;
}TUNINGVAL;

int mem_get_C_steps(TUNINGVAL tv);
int mem_get_L_steps(TUNINGVAL tv);
int mem_get_AntTrx(TUNINGVAL tv);
void tune_fromMem();
uint16_t memadr(uint32_t freq_Hz, int antnum);
void set_antenne(int ant);
void init_storage();

extern int mem_10L;
extern int mem_10C;
extern uint8_t antenne;

TUNINGVAL makeEEdataset();

#endif /* TUNE_STORAGE_H_ */

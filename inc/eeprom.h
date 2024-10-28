/*
 * eeprom.h
 *
 *  Created on: 14.05.2018
 *      Author: kurt
 */

#ifndef EEPROM_H_
#define EEPROM_H_

void init_eeprom();
uint8_t i2c_readbyte(uint16_t adr);
void i2c_sendbyte(uint16_t adr, uint8_t data);
void i2c_sendpage64(uint16_t adr, uint8_t* pdata);
void i2c_readpage64(uint16_t adr, uint8_t* pdata);
void copyMemToEE();
void copyEeToMem();
void saveTuningValue();
TUNINGVAL *readTuningValue();
void clearTuningValue(int freq);
void clearFullEEprom();
void clearBandEEprom();
void checkConfig();

typedef struct
{
	float refWlow;
	float refWhigh;
	float refmVlow_fwd;
	float refmVhigh_fwd;
	float refmVlow_rev;
	float refmVhigh_rev;
	float ref_low_dBm;
	float ref_high_dBm;

	int refU3W, refU100W;
	int refI3W, refI100W;

	uint8_t civ_adr;
} CONFIG;

extern CONFIG eeconfig;
extern uint8_t ee_mem[];
extern uint32_t tunedToFreq;

#define EEPROMADR		0xA0    // = 0x50<<1

#define EESIZE 		32768
#define PAGESIZE 	64
#define PAGES 		EESIZE/PAGESIZE	// 512 Pages

#define EESAVETIME		10000	// alle 10s
#define MAXI2CTXDATA	70	    // 64 Bytes Page + Adress = max 66 Bytes
#define MAGIC 0xfe6692a5
#define FSTEPSIZE		10000	// Schrittweite in Hz pro EEPROM Eintrag


#endif /* EEPROM_H_ */

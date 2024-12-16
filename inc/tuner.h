/*
 * tuner.h
 *
 *  Created on: 11.08.2018
 *      Author: kurt
 */

#ifndef TUNER_H_
#define TUNER_H_

// Namen der Tuningprozesse
typedef enum {
	NO_TUNING,
	GRUNDSTELLUNG,
	SEEK50_PLUSPHI_LOWER_L,		// unterer Smithbereich (normal), sucht 50 Ohm bei +phi durch verkleinern des Ls
	SEEK_PHINEG_LOWER_C,		// sucht Phi < 0 durch Verkleinern des Cs
	SEEK_LOWEST_SWR,		// sucht das kleinste SWR durch SWR Messung
	SEEK_ADJ_L,			// ändere L für kleinstes SWR
	SEEK_PHI_NEG_LOWER_C,		// oberer Smithbereich
	SEEK_Z50_LOWER_C,
	SEEK_PHIPLUS_LOWERL,
	START_TUNING,
	TUNING_DONE,
} tuning_state_t;

#define MAXPROT 180	// maximal so groß wie TXBUF - 4

void tune();
void tune_full();
void tune_fullband();
void set_suchSpeed(int v);
void set_suchVal(int v);
void seek_lowest_swr_extern();
void seek_band();
void seek_single();
void stop_tuning();
void requestTuning(int nr);

extern unsigned int tuningspeed;
extern int do_tuning;
extern int tuning_procnum;
extern int tuningsource;
extern int no_tuneFromMem;
extern int tuning_band;

#endif /* TUNER_H_ */

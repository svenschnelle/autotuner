/*
 * tuner.h
 *
 *  Created on: 11.08.2018
 *      Author: kurt
 */

#ifndef TUNER_H_
#define TUNER_H_

// Namen der Tuningprozesse
enum _TUNINGPROCS_ {
	NO_TUNING = 0,
	GRUNDSTELLUNG,
								// unterer Smithbereich (normal)
	SEEK50_PLUSPHI_LOWER_L,		// sucht 50 Ohm bei +phi durch verkleinern des Ls
	SEEK_PHINEG_LOWER_C,		// sucht Phi < 0 durch Verkleinern des Cs
	SEEK_LOWEST_SWR,			// sucht das kleinste SWR durch SWR Messung
	SEEK_ADJ_L,					// ändere L für kleinstes SWR

								// oberer Smithbereich
	SEEK_PHI_NEG_LOWER_C,
	SEEK_Z50_LOWER_C,
	SEEK_PHIPLUS_LOWERL,

	SEEK_COMPLETE, // TODO: hier entfernen
	MAXTUNINGPROCS
};

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

extern int tuningspeed;
extern int do_tuning;
extern int tuning_procnum;
extern int tuningsource;
extern int no_tuneFromMem;
extern int tuning_band;

#endif /* TUNER_H_ */

/*
 * tune_storage.c
 *
 *  Created on: 28.04.2018
 *      Author: kurt
 */

#include <main.h>

int mem_10L = 11;
int mem_10C = 1;

TUNINGVAL tunval;
uint8_t antenne = 0;

/*
 * Das EEPROM hat 32kB in 512 Pages zu je 64 Byte
 *
 * Platzbedarf (in TUNINGVALs, also 3 Byte) für die Bänder bei 10kHz Schritten
 *
 * Band     Start    Ende       Breite  Datensätze AdrOffset
 * 160m ...  1800 -  2000 kHz ...  200 kHz ...  20 ...    0
 *  80m ...  3500 -  3800 kHz ...  300 kHz ...  30 ...   60
 *  60m ...  5300 -  5400 kHz ...  100 kHz ...  10 ...  150
 *  40m ...  7000 -  7200 kHz ...  200 kHz ...  20 ...  180
 *  30m ... 10100 - 10150 kHz ...   50 kHz ...   5 ...  240
 *  20m ... 14000 - 14350 kHz ...  350 kHz ...  35 ...  255
 *  17m ... 18060 - 18170 kHz ...  110 kHz ...  11 ...  360
 *  15m ... 21000 - 21450 kHz ...  450 kHz ...  45 ...  393
 *  12m ... 24890 - 24990 kHz ...  100 kHz ...  10 ...  528
 *  10m ... 28000 - 29700 kHz ... 1700 kHz ... 170 ...  558 (bis 1067)
 *
 *  jede Antenne braucht also 17 Pages (17*64=1088)
 *
 *  Pages:
 *  0-9 ... div. Konfigurationsdaten
 *  10-26 ... Antenne-1
 *  30-46 ... Antenne-2
 *  50-66 ... Antenne-3
 *  70-86 ... Antenne-4
 *  90-106 ... Antenne-5
 *  110-126 ... Antenne-6
 *
 *
 * TUNINGVAL: Datensatz aus den aktuellen Tunereinstellungen
 * 9 Spulen ... nehme 11 Bit (also 2 in Reserve)
 * 8 Kondensatoren ... nehme 11 Bit (also 3 in Reserve)
 * 2 Relais Ant/Trx ... nehme 2 Bit
 *
 * LC1: l7,l6,l5,l4   l3,l2 ,l1,l0
 * LC2: c4,c3,c2,c1   c0,l10,l9,l8
 * LC3: AN,TR,c10,c9  c8,c7 ,c6,c5
 */

void set_antenne(int ant)
{
	antenne = ant;
}

// Anzahl der Datensätze (je 3 Byte) pro Band
uint32_t mem_datasets[BANDEDGENUM];
// Startadresse pro Band
uint32_t mem_bandstartadr[BANDEDGENUM];

void init_storage()
{
	// speichere Anzahl der Datensätze pro Band
	for(int i=0; i<BANDEDGENUM; i++)
		mem_datasets[i] = (stopedges[i] - startedges[i])/FSTEPSIZE;

	// Speichere Startadresse pro Band (offset zur Antennen-Baseadresse)
	uint32_t adr = 0;
	for(int i=0; i<BANDEDGENUM; i++)
	{
		mem_bandstartadr[i] = adr;

		adr += (mem_datasets[i] * 3);	// 3 Byte pro Datensatz
		adr += 3;						// damit sich Ende und Start nicht überschneiden
	}
}

// berechne Speicheradresse für eine Frequenz
uint16_t memadr(uint32_t freq_Hz, int antnum)
{
uint16_t adr;
int bandoffset=0;
int foffset=0;
int base=10;

	// Basis-Page für eine Antenne
	switch(antnum)
	{
	case 0: base = 10; break;
	case 1: base = 30; break;
	case 2: base = 50; break;
	case 3: base = 70; break;
	case 4: base = 90; break;
	case 5: base = 110; break;
	default: return EESIZE-3; // schreibe Unsinn ans Ende weg
	}

	// offset für das aktuelle Band
	int band = getBandnum(freq_Hz);
	if(band == -1)
		return EESIZE-3; // schreibe Unsinn ans Ende weg

	bandoffset = mem_bandstartadr[band];

	// offset innerhalb des Bandes für die aktuelle Frequenz
	foffset = freq_Hz - startedges[band];
	foffset /= FSTEPSIZE;

	adr = base*PAGESIZE + bandoffset + foffset * 3;

	return adr;
}

// erzeuge einen Datensatz aus den aktuellen Tunerrelaisstellungen
TUNINGVAL makeEEdataset()
{
	int L = store_actL();
	tunval.LC1 = L & 0xff;
	tunval.LC2 = (L >> 8) & 0x07;

	int C = store_actC();
	tunval.LC2 |= (C & 0x1f) << 3;
	tunval.LC3 = (C >> 5) & 0x3f;

	if(relays[L_TRX].state)
		tunval.LC3 |= 0x40;
	if(relays[L_ANT].state)
		tunval.LC3 |= 0x80;

	return tunval;
}

// lese die Relaisstellungen aus einem Datensatz
int mem_get_C_steps(TUNINGVAL tv)
{
int C;

	C = (tv.LC2 >> 3) & 0x1f;
	C += (tv.LC3 & 0x3f) << 5;

	return C;
}

int mem_get_L_steps(TUNINGVAL tv)
{
int L;

	L = tv.LC1;
	L += (tv.LC2 & 0x07) << 8;

	return L;
}

// 0=off 1=ant 2=trx 3=both
int mem_get_AntTrx(TUNINGVAL tv)
{
int ret = 0;

	if(tv.LC3 & 0x40) ret |= 2;
	if(tv.LC3 & 0x80) ret |= 1;

	return ret;
}

// stelle Tuner für die aktuelle Frequenz aus dem Memory ein
void tune_fromMem()
{
	TUNINGVAL *tv = readTuningValue();

	if(tv == NULL) return;	// kein Eintrag oder keine Frequenz verfügbar

	int L = mem_get_L_steps(*tv);
	int C = mem_get_C_steps(*tv);
	int anttrx = mem_get_AntTrx(*tv);

	set_L_step(L);
	set_C_step(C);

	if(anttrx == 1)
	{
		switch_t2relais(L_ANT, ON);
		switch_t2relais(L_TRX, OFF);
	}
	if(anttrx == 2)
	{
		switch_t2relais(L_ANT, OFF);
		switch_t2relais(L_TRX, ON);
	}
}

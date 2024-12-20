/*
 * setL.c
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

/*
 * schaltet ein gewünschtes L so genau wie möglich ein
 */

#include <main.h>

#define COILANZ	L12800 - L50 + 1
static const int coils[COILANZ] = {50, 100, 200, 400, 800, 1600, 3200, 6400, 12800};
static const int eigenL = 0;	// nH, Grundinduktivität der Leitungen

// suche die L-Relais für einen gewünschten Wert in nH
// dann schalte die entsprechenden Relais
void set_L(int nH)
{
	if(nH <= eigenL) {
		set_L_step(0);
		return;
	}

	for(int i=0; i<512; i++) {
		int res = eigenL;
		for (int v = 0; v < COILANZ; v++) {
			if(i & (1<<v))
				res += coils[v];
		}

		if((nH > (res - 25)) && (nH < (res + 25))) {
			// gefunden, Relaisstellungen sind in i
			set_L_step(i);
			return;
		}
	}
}

void set_L_step(int step)
{
	if(step < 0)
		return;

	if(step >= ANZ_L_STUFEN)
		return;

	switch_t2relais(L50,    (step & 1) == 0);
	switch_t2relais(L100,   (step & 2) == 0);
	switch_t2relais(L200,   (step & 4) == 0);
	switch_t2relais(L400,   (step & 8) == 0);
	switch_t2relais(L800,   (step & 16) == 0);
	switch_t2relais(L1600,  (step & 32) == 0);
	switch_t2relais(L3200,  (step & 64) == 0);
	switch_t2relais(L6400,  (step & 128) == 0);
	switch_t2relais(L12800, (step & 256) == 0);
}

// schalte die Ls zur Antenne ant_trx=1 oder an den Transceiver ant_trx=0, 2=kein Anschluss (Messbetrieb)
void pos_L(int ant_trx)
{
	switch (ant_trx) {
	case 0:
		switch_t2relais(L_TRX,ON);
		switch_t2relais(L_ANT,OFF);
		break;
	case 1:
		switch_t2relais(L_TRX,OFF);
		switch_t2relais(L_ANT,ON);
		break;
	case 2:
		switch_t2relais(L_TRX,OFF);
		switch_t2relais(L_ANT,OFF);
		break;
	default:
		break;
	}
}

int calc_l()
{
	int l = 0;

	for(int i = 0; i < COILANZ; i++)
		if(relays[i + L50].state == 0)
			l += coils[i];

	return l + eigenL;
}

int store_actL()
{
	int actLpos = 0;

	if(relays[L50].state == 0)
		actLpos += 1 ;

	if(relays[L100].state == 0)
		actLpos += 2 ;

	if(relays[L200].state == 0)
		actLpos += 4 ;

	if(relays[L400].state == 0)
		actLpos += 8 ;

	if(relays[L800].state == 0)
		actLpos += 16 ;

	if(relays[L1600].state == 0)
		actLpos += 32 ;

	if(relays[L3200].state == 0)
		actLpos += 64 ;

	if(relays[L6400].state == 0)
		actLpos += 128 ;

	if(relays[L12800].state == 0)
		actLpos += 256 ;

	return actLpos;
}

void updn_Lstep(int num)
{
	int newLpos = store_actL() + num;

	if(newLpos < 0)
		newLpos = 0;

	if(newLpos > 255)
		newLpos = 255;

	set_L_step(newLpos);
}

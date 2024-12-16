/*
 * setC.c
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

/*
 * schaltet ein gewünschtes C so genau wie möglich, aber immer kleiner als
 * der gesuchte Wert
 * der Rest wird dann mit dem Drehko hinzugefügt
 */

#include <main.h>

#define KAPANZ	C3200 - C25 + 1
static const int caps[KAPANZ] = {25, 50, 100, 200, 400, 800, 1600, 3200};
static const int eigenC = 0;	// pF, zusätzliche Verdrahtungskapazität

void set_C(int pF)
{
	if(pF < eigenC) {
		set_C_step(0);
		return;
	}

	for(int i = 0; i < 256; i++) {
		int res = eigenC;
		for(int v = 0; v < KAPANZ; v++) {
			if(i & ( 1 << v))
				res += caps[v];
		}

		if(pF > (res - 12) && pF < ( res + 13)) {
			// gefunden, Relaisstellungen sind in i
			set_C_step(i);
			return;
		}
	}
}

void set_C_step(unsigned int step)
{
	if (step == 0xffffffffU) {
		printinfo("C step invalid\n");
			  for(;;);
	}

	switch_t2relais(C25,   (step & 0x01) ? 1 : 0);
	switch_t2relais(C50,   (step & 0x02) ? 1 : 0);
	switch_t2relais(C100,  (step & 0x04) ? 1 : 0);
	switch_t2relais(C200,  (step & 0x08) ? 1 : 0);
	switch_t2relais(C400,  (step & 0x10) ? 1 : 0);
	switch_t2relais(C800,  (step & 0x20) ? 1 : 0);
	switch_t2relais(C1600, (step & 0x40) ? 1 : 0);
	switch_t2relais(C3200, (step & 0x80) ? 1 : 0);
}

uint8_t store_actC()
{
	uint8_t actCpos = 0;

	if(relays[C25].state == 1)
		actCpos += 1;

	if(relays[C50].state == 1)
		actCpos += 2;

	if(relays[C100].state == 1)
		actCpos += 4;

	if(relays[C200].state == 1)
		actCpos += 8;

	if(relays[C400].state == 1)
		actCpos += 16;

	if(relays[C800].state == 1)
		actCpos += 32;

	if(relays[C1600].state == 1)
		actCpos += 64;

	if(relays[C3200].state == 1)
		actCpos += 128;

	return actCpos;
}

int calc_c()
{
	int c = 0;

	for(int i = 0; i < KAPANZ; i++)
		if(relays[i + C25].state == 1)
			c += caps[i];

	return c + eigenC; // nur halben C wegen Serienschaltung
}

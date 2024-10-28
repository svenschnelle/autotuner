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
int caps[KAPANZ] = {25,50,100,200,400,800,1600,3200};

int actC = 0;

int eigenC = 0;	// pF, zusätzliche Verdrahtungskapazität

// stelle Relais-C so ein, dass es um einen Schritt vor dem gewünschten C ist
// den Rest kann der Drehko machen
void set_C(int pF)
{
	// den Wert verdoppeln, weil wir 2 Kondensatorbänke in Reihe haben
	pF *= 2;

	if(pF < eigenC)
	{
		set_C_step(0);
		return;
	}

	for(int i=0; i<256; i++)
	{
		int res = eigenC;
		for(int v=0; v<KAPANZ; v++)
		{
			if((i & (1<<v)) != 0)
			{
				res += caps[v];
			}
		}

		if((pF > (res-12)) && (pF < (res+13)))
		{
			// gefunden, Relaisstellungen sind in i
			set_C_step(i);
			return;
		}
	}
}

void set_C_step(int step)
{
	switch_t2relais(C25, (step & 1)?1:0);
	switch_t2relais(C50, (step & 2)?1:0);
	switch_t2relais(C100, (step & 4)?1:0);
	switch_t2relais(C200, (step & 8)?1:0);
	switch_t2relais(C400, (step & 0x10)?1:0);
	switch_t2relais(C800, (step & 0x20)?1:0);
	switch_t2relais(C1600, (step & 0x40)?1:0);
	switch_t2relais(C3200, (step & 0x80)?1:0);
}


uint8_t store_actC()
{
uint8_t actCpos = 0;

	if(relpos[C25] == 1) actCpos +=1 ;
	if(relpos[C50] == 1) actCpos +=2 ;
	if(relpos[C100] == 1) actCpos +=4 ;
	if(relpos[C200] == 1) actCpos +=8 ;
	if(relpos[C400] == 1) actCpos +=16 ;
	if(relpos[C800] == 1) actCpos +=32 ;
	if(relpos[C1600] == 1) actCpos +=64 ;
	if(relpos[C3200] == 1) actCpos +=128 ;

	return actCpos;
}

int calc_c()
{
int c = 0;

	for(int i=0; i<KAPANZ; i++)
		if(relpos[i+C25] == 1) c+= caps[i];

	return c/2 + eigenC; // nur halben C wegen Serienschaltung
}

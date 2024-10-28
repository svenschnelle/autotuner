/*
 * bandcalc.c
 *
 *  Created on: 31.05.2018
 *      Author: kurt
 */

#include <main.h>

uint32_t startedges[BANDEDGENUM] =
{
		1800000,
		3500000,
		5300000,
		7000000,
		10100000,
		14000000,
		18060000,
		21000000,
		24890000,
		28000000
};

uint32_t stopedges[BANDEDGENUM] =
{
		2000000,
		3800000,
		5400000,
		7200000,
		10150000,
		14350000,
		18170000,
		21450000,
		24990000,
		29700000
};

int getBandnum(uint32_t freq)
{
	for(int i=0; i<BANDEDGENUM; i++)
	{
		if(freq <= stopedges[i])
			return i;
	}
	return -1;
}

void getFreqEdges(uint32_t freq , uint32_t *start, uint32_t *stop)
{
	// prüfe ob freq in einem Band liegt
	int inband = 0;
	for(int i=0; i<BANDEDGENUM; i++)
	{
		if(freq >= startedges[i] && freq <= stopedges[i])
		{
			inband = 1;
			break;
		}
	}

	if(inband == 0)
	{
		// wenn nicht im Band, nehme nächst höheres Band
		for(int i=0; i<BANDEDGENUM; i++)
		{
			if(startedges[i] > freq)
			{
				freq = startedges[i];
				break;
			}
		}
	}

	// Suche Eckfrequenzen
	for(int i=0; i<BANDEDGENUM; i++)
	{
		if(freq >= startedges[i] && freq <= stopedges[i])
		{
			*start = startedges[i];
			*stop = stopedges[i];
			return;
		}
	}

	// kann hier nicht herkommen
	*start = 0;
	*stop = 0;
}

// stelle fest für welche Bänder Speicherinhalte vorhanden sind
uint8_t mem_avail[BANDEDGENUM];

uint8_t *get_MemUsage()
{
	for(int band = 0; band < BANDEDGENUM; band++)
	{
		int qrgs = 0;
		int mems = 0;

		for(int qrg = startedges[band]; qrg <= stopedges[band]; qrg += 10000)
		{
			qrgs++;

			// gibt es einen Wert an der aktuellen Position ?
			uint16_t adr = memadr(qrg,antenne);
			if(ee_mem[adr+1] != 0xff)
				mems++;	// Wert vorhanden
		}

		mem_avail[band] = (mems * 100)/qrgs;
	}

	return mem_avail;
}

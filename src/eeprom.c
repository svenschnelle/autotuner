/*
 * eeprom.c
 *
 *  Created on: 14.05.2018
 *      Author: kurt
 */

/*
 * I2C3:
 * SCL ... PA8
 * SDA ... PC9
 *
 * GPIO out:
 * WP .... PC8
 */

#include <main.h>
#include <i2c.h>

void init_eeprom()
{
	copyEeToMem();
}

// sende eine 64 Byte Page

// ============= EEPROM Funktionen ===================

/* Das EEPROM hat 256kbit = 32kByte mit 64 Byte Pages
 * das sind 512 Pages
 *
 */

// markiert eine Modifikation der Page damit diese gespeichert wird
static uint8_t page_modified[PAGES];

// Speicherabbild des eeproms
uint8_t ee_mem[EESIZE];

// Konfiguration für Page 0
CONFIG eeconfig;

// schreibt geänderte Pages ins EEPROM
void copyMemToEE()
{
	for(int page = 0; page < PAGES; page++)	{
		if(page_modified[page])	{
			page_modified[page] = 0;
			i2c_sendpage64(EEPROMADR, page*PAGESIZE, ee_mem+page*PAGESIZE);
		}
	}
}

// kopiert die Config ins eemem
void configToEEmem()
{
	uint8_t *p = (uint8_t *)(&eeconfig);

	uint16_t crc16 = crc16_messagecalc(p, sizeof(CONFIG));

	int idx = 0;
	for(unsigned int i = 0; i<sizeof(CONFIG); i++)
		ee_mem[idx++] = p[i];

	ee_mem[idx++] = crc16 >> 8;
	ee_mem[idx] = crc16;

	for(unsigned int i = 0; i<=(sizeof(CONFIG) / 64); i++)
		page_modified[i] = 1;
}

void EEmemToConfig()
{
	uint16_t crc16;
	uint8_t *p = (uint8_t *)(&eeconfig);

	int idx = 0;
	for(unsigned int i=0; i < sizeof(CONFIG); i++)
		p[i] = ee_mem[idx++];

	crc16 = ee_mem[idx++];
	crc16 <<= 8;
	crc16 += ee_mem[idx];

	uint16_t crc = crc16_messagecalc(p, sizeof(CONFIG));
	if(crc != crc16)
	{
		printinfo("EEprom CRC Error");
	}
	else
	{

		printinfo("EEprom Config OK");
	}
}

// checksumme der Config
int calcConfigSum()
{
	uint8_t *p = (uint8_t *)(&eeconfig);
	int sum = 0;

	for(unsigned int i=0; i < sizeof(CONFIG); i++)
		sum += p[i];

	return sum;
}

// überwache die Config, wenn geändert, schreibe sie ins eeprom
void checkConfig()
{
	static int first = 1;
	static int cfgsum;

	if(first)
	{
		cfgsum = calcConfigSum();
		first = 0;
	}
	else
	{
		int s = calcConfigSum();
		if(s != cfgsum)
		{
			configToEEmem();
			cfgsum = s;
		}
	}
}

// liest das gesamte eeprom in den Speicher (einmal nach dem Einschalten)
void copyEeToMem()
{
	for(int page = 0; page < PAGES; page++) {
		i2c_readpage64(EEPROMADR, page * PAGESIZE, ee_mem + page * PAGESIZE);
		page_modified[page] = 0;
	}

	EEmemToConfig();

	// check EEPROM (4 Bytes in Page 511 sind die Magic Nummer)
	if(	ee_mem[511*PAGESIZE+0] != (MAGIC & 0xff) ||
		ee_mem[511*PAGESIZE+1] != ((MAGIC>>8) & 0xff) ||
		ee_mem[511*PAGESIZE+2] != ((MAGIC>>16) & 0xff) ||
		ee_mem[511*PAGESIZE+3] != ((MAGIC>>24) & 0xff))
	{
		// EEprom neu oder Fehler
		// schreibe Magic neu, diese befindet sich in Page 511
		ee_mem[511*PAGESIZE+0] = MAGIC & 0xff;
		ee_mem[511*PAGESIZE+1] = (MAGIC>>8) & 0xff;
		ee_mem[511*PAGESIZE+2] = (MAGIC>>16) & 0xff;
		ee_mem[511*PAGESIZE+3] = (MAGIC>>24) & 0xff;
		page_modified[511] = 1;

		// Lösche Tuningspeicher
		for(int i=10*PAGESIZE; i<127*PAGESIZE; i++)
			ee_mem[i] = 0xff;

		for(int i=10; i<127; i++)
			page_modified[i] = 1;

		// init Config
		eeconfig.refWlow = 3;
		eeconfig.refWhigh = 100;
		eeconfig.ref_low_dBm = 34;
		eeconfig.ref_high_dBm = 50;
		eeconfig.refmVlow_fwd = 1388;
		eeconfig.refmVhigh_fwd = 1683;
		eeconfig.refmVlow_rev = 1355;
		eeconfig.refmVhigh_rev = 1660;
		eeconfig.refU3W = 1300;
		eeconfig.refU100W = 1700;
		eeconfig.refI3W = 1100;
		eeconfig.refI100W = 1600;
		eeconfig.civ_adr = 0x94;
		configToEEmem();
	}
}

// speichert einen Tuningwert ins EEPROM-Memory
void saveTuningValue()
{
	uint32_t civ_freq = getCIVfreq();

	tuningsource = 0;

	if(civ_freq == 0)
		return;

	TUNINGVAL tv = makeEEdataset();
	uint16_t adr = memadr(civ_freq,antenne);

	ee_mem[adr] = tv.LC1;
	ee_mem[adr+1] = tv.LC2;
	ee_mem[adr+2] = tv.LC3;

	printinfo("save %d: %d C%02X L%02X\n", adr, civ_freq / 1000, store_actC(), store_actL());

	page_modified[adr/PAGESIZE] = 1;
	page_modified[(adr+1)/PAGESIZE] = 1;	// falls es zur nächsten Page weitergeht
	page_modified[(adr+2)/PAGESIZE] = 1;
}

void clearTuningValue(int freq)
{
	uint16_t adr = memadr(freq,antenne);

	ee_mem[adr] = 0xff;
	ee_mem[adr+1] = 0xff;
	ee_mem[adr+2] = 0xff;

	page_modified[adr/PAGESIZE] = 1;
}

void clearFullEEprom()
{
	// EEprom neu oder Fehler
	for(int i=0; i<EESIZE; i++)
		ee_mem[i] = 0xff;

	ee_mem[0] = MAGIC & 0xff;
	ee_mem[1] = (MAGIC>>8) & 0xff;
	ee_mem[2] = (MAGIC>>16) & 0xff;
	ee_mem[3] = (MAGIC>>24) & 0xff;
	for(int i=0; i<PAGES; i++)
		page_modified[i] = 1;
}

// lösche alle Werte des aktuellen Bandes
void clearBandEEprom()
{
	uint32_t civ_freq = getCIVfreq();

	if(civ_freq == 0)
		return;

	uint32_t startfreq, stopfreq;
	getFreqEdges(civ_freq , &startfreq, &stopfreq);

	// lösche das komplette Bandmemory
	for(uint32_t freq = startfreq; freq < stopfreq; freq+=FSTEPSIZE)
	{
		clearTuningValue(freq);
	}
}

TUNINGVAL eetv;
uint32_t tunedToFreq = 0;	// Frequenz für die ein Tuning gefunden wurde

TUNINGVAL *readTuningValue()
{
	uint32_t civ_freq = getCIVfreq();

	if(civ_freq == 0)
		return NULL;

	uint32_t civ_startfreq = (civ_freq / 10000) * 10000;	// entferne Kommastellen unterhalb 10kHz
	tuningsource = 1;

	// gibt es einen Wert an der aktuellen Position ?
	uint16_t adr = memadr(civ_freq,antenne);
	if(ee_mem[adr+1] != 0xff)
	{
		// Wert vorhanden, nehme diesen
		eetv.LC1 = ee_mem[adr];
		eetv.LC2 = ee_mem[adr+1];
		eetv.LC3 = ee_mem[adr+2];
		tunedToFreq = civ_startfreq;
		return &eetv;
	}

	// kein Wert gefunden, suche innerhalb des Bandes den nächstgelegenen
	int band = getBandnum(civ_freq);
	if(band == -1)
	{
		eetv.LC2 = 0xff;	// nichts gefunden
		tuningsource = 0;
		tunedToFreq = 0;
		return NULL;
	}

	uint32_t start = startedges[band];
	uint32_t stop = stopedges[band];
	uint32_t upper = 0;
	uint32_t lower = 0;
	uint16_t upper_adr = 0;
	uint16_t lower_adr = 0;

	// suche höhere
	for(uint32_t freq = civ_startfreq; freq <= stop; freq += FSTEPSIZE)
	{
		adr = memadr(freq,antenne);
		if(ee_mem[adr+1] != 0xff)
		{
			upper = freq;
			upper_adr = adr;
			break;
		}
	}

	// suche niedrigere
	for(uint32_t freq = civ_startfreq; freq >= start; freq -= FSTEPSIZE)
	{
		adr = memadr(freq,antenne);
		if(ee_mem[adr+1] != 0xff)
		{
			lower = freq;
			lower_adr = adr;
			break;
		}
	}

	if(upper == 0 && lower == 0)
	{
		eetv.LC2 = 0xff;	// nichts gefunden
		tuningsource = 0;
		tunedToFreq = 0;
		return NULL;
	}

	if(upper != 0 && lower != 0)
	{
		// suche besseren von beiden
		if((upper - civ_freq) < (civ_freq - lower))
			lower = 0;
		else
			upper = 0;
	}

	if(upper != 0 && lower == 0)
	{
		eetv.LC1 = ee_mem[upper_adr];
		eetv.LC2 = ee_mem[upper_adr+1];
		eetv.LC3 = ee_mem[upper_adr+2];
		tunedToFreq = upper;
		return &eetv;
	}

	if(upper == 0 && lower != 0)
	{
		eetv.LC1 = ee_mem[lower_adr];
		eetv.LC2 = ee_mem[lower_adr+1];
		eetv.LC3 = ee_mem[lower_adr+2];
		tunedToFreq = lower;
		return &eetv;
	}

	eetv.LC2 = 0xff;	// nichts gefunden
	tuningsource = 0;
	tunedToFreq = 0;
	return NULL;
}

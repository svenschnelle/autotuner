/*
 * remote.c
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#include <main.h>

/*
 * Fernsteuerung des Tuners via serieller Schnittstelle
 *
 * Datenformat zum Tuner:
 * 0,1,2,3: Header: 0x45 0x85 0xf4 0xa7
 * 4:       Type: C (schalte C ein/aus) , L (schalte L ein/aus), M (schalte L auf TRX oder ANT), S (bediene Stepper)
 * 5:       Value High, oder Zusatzinfo zu Byte 4
 * 6:       Value Low , oder weitere Zusatzinfo zu Byte 4
 * 7:		CRC16 high
 * 8:		CRC16 low
 */

#define MAXURXDATA 500

uint8_t remoteCmdValid = 0;
uint16_t rxcrc16;
uint16_t ulen;
uint8_t urxdata[MAXURXDATA];
uint16_t urxlen;

// empfange Kommandos via ser. Schnittstelle
// ! wird aus dem RX IRQ aufgerufen !
void handle_remoteData(uint8_t data)
{
static int state = 0;

	if(remoteCmdValid == 1) return;  // letztes Kommando noch nicht verarbeitet

	switch (state)
	{
	case 0: if(data == 0x45) state = 1;
			break;
	case 1: if(data == 0x85) state = 2;
			else state = 0;
			break;
	case 2: if(data == 0xf4) state = 3;
			else state = 0;
			break;
	case 3: if(data == 0xa7) state = 4;
			else state = 0;
			break;
	case 4:
			ulen = data;
			ulen <<= 8;
			state = 5;
			break;
	case 5: ulen += data;
			urxlen = 0;
			state = 6;
			break;
	case 6:	urxdata[urxlen++] = data;
			if(urxlen >= MAXURXDATA || urxlen == ulen)
				state = 100;
			break;
	case 100:
			rxcrc16 = data;
			rxcrc16 <<= 8;
			state = 101;
			break;
	case 101:
			rxcrc16 += data;
			uint16_t crc16 = crc16_messagecalc(urxdata,ulen);
			if(rxcrc16 == crc16)
			{
				remoteCmdValid = 1;
			}
			state = 0;
			break;
	}
}

// wird aus der Hauptschleife aufgerufen um empfangene Remote Kommandos auszuführen
void execute_Remote()
{
uint8_t type = urxdata[0];
uint8_t zinfo1 = urxdata[1];
uint8_t zinfo2 = urxdata[2];
uint16_t value;

	value = zinfo1;
	value <<= 8;
	value += zinfo2;

	if(remoteCmdValid == 1)
	{
		remoteCmdValid = 0;
		if(type == 'C')
		{
			switch (zinfo1)
			{
			case 'A': 	for(int i=C25; i<=C3200; i++)
							switch_t2relais(i, OFF);
						break;
			case 'E': 	for(int i=C25; i<=C3200; i++)
							switch_t2relais(i, ON);
						break;
			case 'V': 	for(int i=C25; i<=C3200; i++)
						{
							if(zinfo2 & (1<<(i-C25)))
								switch_t2relais(i, ON);
							else
								switch_t2relais(i, OFF);
						}
						break;
			case '1':	switch_t2relais(C25, TOGGLE);
						break;
			case '2':	switch_t2relais(C50, TOGGLE);
						break;
			case '3':	switch_t2relais(C100, TOGGLE);
						break;
			case '4':	switch_t2relais(C200, TOGGLE);
						break;
			case '5':	switch_t2relais(C400, TOGGLE);
						break;
			case '6':	switch_t2relais(C800, TOGGLE);
						break;
			case '7':	switch_t2relais(C1600, TOGGLE);
						break;
			case '8':	switch_t2relais(C3200, TOGGLE);
						break;
			}
		}

		if(type == 'L')
		{
			switch (zinfo1)
			{
			case 'A': 	for(int i=L50; i<=L12800; i++)
							switch_t2relais(i, ON);
						break;
			case 'E': 	for(int i=L50; i<=L12800; i++)
							switch_t2relais(i, OFF);
						break;
			case '1':	switch_t2relais(L50, TOGGLE);
						break;
			case '2':	switch_t2relais(L100, TOGGLE);
						break;
			case '3':	switch_t2relais(L200, TOGGLE);
						break;
			case '4':	switch_t2relais(L400, TOGGLE);
						break;
			case '5':	switch_t2relais(L800,TOGGLE);
						break;
			case '6':	switch_t2relais(L1600, TOGGLE);
						break;
			case '7':	switch_t2relais(L3200, TOGGLE);
						break;
			case '8':	switch_t2relais(L6400, TOGGLE);
						break;
			case '9':	switch_t2relais(L12800, TOGGLE);
						break;
			}
		}

		if(type == 'I')
		{
			for(int i=L50; i<=L12800; i++)
			{
				if(value & (1<<(i-L50)))
					switch_t2relais(i, OFF);
				else
					switch_t2relais(i, ON);
			}
		}

		if(type == 'M')
		{
			switch (zinfo1)
			{
			case 'O': 	for(int i=L_TRX; i<=L_ANT; i++)
							switch_t2relais(i, OFF);
						break;
			case 'T':	switch_t2relais(L_TRX, TOGGLE);
						break;
			case 'A':	switch_t2relais(L_ANT, TOGGLE);
						break;
			case 'S':	if(zinfo2 == 1)
						{
							switch_t2relais(L_ANT, ON);
							switch_t2relais(L_TRX, OFF);
						}
						if(zinfo2 == 2)
						{
							switch_t2relais(L_ANT, OFF);
							switch_t2relais(L_TRX, ON);
						}
						break;
			}
		}

		if(type == 'X')
		{
			set_C(value);
		}

		if(type == 'Y')
		{
			set_L(value);
		}

		if(type == 'W')
		{
			set_suchVal(value);
		}

		if(type == 'R')
		{
			set_suchSpeed(value);
		}

		if(type == 'T')
		{
			switch (zinfo1)
			{
			case 'T':	cat_txrx(zinfo2);
						break;
			case 'M':	cat_mode(zinfo2);
						break;
			case 'F':	cat_setqrg(urxdata+2);
						last_freq = 0;
						break;
			}
		}

		if(type == 'K')
		{
			switch (zinfo1)
			{
			case 'E':	cal_3watt();
						break;
			case 'H':	cal_100watt();
						break;
			case 'C':	set_civ(zinfo2);
						break;
			}
		}

		if(type == 'A')
		{
			switch (zinfo1)
			{
			case 'G':
				requestTuning(GRUNDSTELLUNG);
				break;
			case '5':
				requestTuning(SEEK50_PLUSPHI_LOWER_L);
				break;
			case '0':
				requestTuning(SEEK_PHINEG_LOWER_C);
				break;
			case 'S':
				requestTuning(SEEK_LOWEST_SWR);
				break;
			case 'A':
				seek_single();
				break;
			case 'X':
				requestTuning(SEEK_PHI_NEG_LOWER_C);
				break;
			case 'Y':
				requestTuning(SEEK_Z50_LOWER_C);
				break;
			case 'Z':
				requestTuning(SEEK_PHIPLUS_LOWERL);
				break;
			case 'M':
				seek_band();
				break;
			case 'L':
				stop_tuning();
				break;
			case '1':
				cat_set_outpower(zinfo2);
				break;

			}
		}

		if(type == 'H')
		{
			switch (zinfo1)
			{
			case 'H':
				helloFromGUI();
				break;
			}
		}

		// Memory Funktionen
		if(type == 'E')
		{
			switch (zinfo1)
			{
			case 'B':
				clearBandEEprom();
				break;
			case 'F':
				clearFullEEprom();
				break;
			case 'A':
				set_antenne(zinfo2);
				break;
			case 'S':
				saveTuningValue();
				break;
			}
		}
	}
}

#define INFOTEXTSIZE 40
char infotext[INFOTEXTSIZE+1] = {0};	// Text der seriell übertragen wird

void printinfo(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(infotext, INFOTEXTSIZE, fmt, args);
    va_end(args);

    remote_tx(4, (uint8_t *)infotext, strlen(infotext));
}

// sende Messwerte via serieller Schnittstelle
void send_values_serial()
{
uint8_t txbuf[50];
int idx = 0;

	if(free_fifo_size() < 3)
		return;

	// diese Daten nicht so oft, weil es zuviele sind
	// wenn die analogen Werte wackeln
	if(uart_tick > 0) return;

	uart_tick = 500;

	int c = calc_c();
	int l = calc_l();
	int iswr = (int)(fswr * 100);
	if(iswr > 65000) iswr = 65000;
	int ipwr = (int)(fwd_watt * 10);
	if(ipwr > 65000) ipwr = 65000;

	// sende Memorybelegung
	uint8_t *pu = get_MemUsage();	// Datenlänge 10
	remote_tx(1, pu, 10);

	// sende analoge Messwerte (21 lang)
	idx = 0;
	txbuf[idx++] = ant_U >> 8;	// Antennenspannung
	txbuf[idx++] = ant_U;
	txbuf[idx++] = ant_I >> 8;	// Antennenstrom
	txbuf[idx++] = ant_I;
	txbuf[idx++] = tuningsource;// getuned=0 oder vom Memory=1
	txbuf[idx++] = realZ;
	txbuf[idx++] = phi;
	txbuf[idx++] = antenne;
	txbuf[idx++] = c >> 8;
	txbuf[idx++] = c;
	txbuf[idx++] = l >> 8;
	txbuf[idx++] = l;
	txbuf[idx++] = iswr >> 8;
	txbuf[idx++] = iswr;
	txbuf[idx++] = ipwr >> 8;
	txbuf[idx++] = ipwr;
	txbuf[idx++] = civ_freq >> 24;
	txbuf[idx++] = civ_freq >> 16;
	txbuf[idx++] = civ_freq >> 8;
	txbuf[idx++] = civ_freq;
	txbuf[idx++] = eeconfig.civ_adr;
	txbuf[idx++] = req_freq;
	txbuf[idx++] = relpos[0];
	txbuf[idx++] = tunedToFreq >> 24;
	txbuf[idx++] = tunedToFreq >> 16;
	txbuf[idx++] = tunedToFreq >> 8;
	txbuf[idx++] = tunedToFreq;
	txbuf[idx++] = itemp & 0xff;
	txbuf[idx++] = meas_UB >> 8;
	txbuf[idx++] = meas_UB;
	remote_tx(2, txbuf, idx);

	// Stellung der Relais (Länge 19)
	remote_tx(3, relpos, RELAISANZAHL);
}

void send_prot(uint16_t qrg, uint16_t swr, uint16_t L, uint8_t C)
{
uint8_t txbuf[7];

	txbuf[0] = qrg >> 8;
	txbuf[1] = qrg;

	txbuf[2] = swr >> 8;
	txbuf[3] = swr;

	txbuf[4] = L >> 8;
	txbuf[5] = L;

	txbuf[6] = C;

	while(free_fifo_size() < 1)
	{
		send_serial_fifo();
	}

	remote_tx(0, txbuf, 7);
}

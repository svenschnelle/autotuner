/*
 * remote.c
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#include <main.h>
#include <sys/endian.h>
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

static volatile uint8_t remoteCmdValid = 0;
static volatile uint8_t urxdata[MAXURXDATA];

// empfange Kommandos via ser. Schnittstelle
// ! wird aus dem RX IRQ aufgerufen !
void handle_remoteData(uint8_t data)
{
	static uint16_t rxcrc16 = 0, urxlen, ulen = 0;
	static int state = 0;

	if(remoteCmdValid == 1)
		return;  // letztes Kommando noch nicht verarbeitet

	switch (state) {
	case 0:
		if(data == 0x45)
			state++;
		break;
	case 1:
		if(data == 0x85)
			state++;
		else
			state = 0;
		break;
	case 2:
		if(data == 0xf4)
			state++;
		else
			state = 0;
		break;
	case 3:
		if(data == 0xa7)
			state++;
		else
			state = 0;
		break;
	case 4:
		ulen = data;
		ulen <<= 8;
		state++;
		break;
	case 5:
		ulen += data;
		urxlen = 0;
		state++;
		break;
	case 6:
		urxdata[urxlen++] = data;
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
		uint16_t crc16 = crc16_messagecalc((void *)urxdata,ulen);
		if(rxcrc16 == crc16)
			remoteCmdValid = 1;
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

	if(remoteCmdValid != 1)
		return;

	remoteCmdValid = 0;
	if(type == 'C')	{
		switch (zinfo1)	{
		case 'A':
			for(int i = C25; i <= C3200; i++)
				switch_t2relais(i, OFF);
			break;
		case 'E':
			for(int i = C25; i <= C3200; i++)
				switch_t2relais(i, ON);
			break;
		case 'V':
			for(int i = C25; i <= C3200; i++) {
				if(zinfo2 & (1 << (i - C25)))
					switch_t2relais(i, ON);
				else
					switch_t2relais(i, OFF);
			}
			break;
		case '1':
			switch_t2relais(C25, TOGGLE);
			break;
		case '2':
			switch_t2relais(C50, TOGGLE);
			break;
		case '3':
			switch_t2relais(C100, TOGGLE);
			break;
		case '4':
			switch_t2relais(C200, TOGGLE);
			break;
		case '5':
			switch_t2relais(C400, TOGGLE);
			break;
		case '6':
			switch_t2relais(C800, TOGGLE);
			break;
		case '7':
			switch_t2relais(C1600, TOGGLE);
			break;
		case '8':
			switch_t2relais(C3200, TOGGLE);
			break;
		}
	}

	if(type == 'L')	{
		switch (zinfo1)	{
		case 'A':
			for(int i = L50; i <= L12800; i++)
				switch_t2relais(i, ON);
			break;
		case 'E':
			for(int i = L50; i <= L12800; i++)
				switch_t2relais(i, OFF);
			break;
		case '1':
			switch_t2relais(L50, TOGGLE);
			break;
		case '2':
			switch_t2relais(L100, TOGGLE);
			break;
		case '3':
			switch_t2relais(L200, TOGGLE);
			break;
		case '4':
			switch_t2relais(L400, TOGGLE);
			break;
		case '5':
			switch_t2relais(L800,TOGGLE);
			break;
		case '6':
			switch_t2relais(L1600, TOGGLE);
			break;
		case '7':
			switch_t2relais(L3200, TOGGLE);
			break;
		case '8':
			switch_t2relais(L6400, TOGGLE);
			break;
		case '9':
			switch_t2relais(L12800, TOGGLE);
			break;
		}
	}

	if(type == 'I')	{
		for(int i = L50; i <= L12800; i++) {
			if(value & (1 << (i - L50)))
				switch_t2relais(i, OFF);
			else
				switch_t2relais(i, ON);
		}
	}

	if(type == 'M')	{
		switch (zinfo1)	{
		case 'O':
			for(int i=L_TRX; i <= L_ANT; i++)
				switch_t2relais(i, OFF);
			break;
		case 'T':
			switch_t2relais(L_TRX, TOGGLE);
			break;
		case 'A':
			switch_t2relais(L_ANT, TOGGLE);
			break;
		case 'S':
			if(zinfo2 == 1)	{
				switch_t2relais(L_ANT, ON);
				switch_t2relais(L_TRX, OFF);
			}

			if(zinfo2 == 2) {
				switch_t2relais(L_ANT, OFF);
				switch_t2relais(L_TRX, ON);
			}
			break;
		}
	}

		if(type == 'X')
			set_C(value);

		if(type == 'Y')
			set_L(value);

		if(type == 'W')
			set_suchVal(value);

		if(type == 'R')
			set_suchSpeed(value);

		if(type == 'K')	{
			switch (zinfo1)	{
			case 'E':
				cal_3watt();
				break;
			case 'H':
				cal_100watt();
				break;
			}
		}

		if(type == 'A')	{
			switch (zinfo1) {
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
			}
		}

		if(type == 'H')	{
			switch (zinfo1)	{
			case 'H':
				helloFromGUI();
				break;
			}
		}

		// Memory Funktionen
		if(type == 'E')	{
			switch (zinfo1) {
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

#define INFOTEXTSIZE 80
static char infotext[INFOTEXTSIZE+1] = {0};	// Text der seriell übertragen wird

void printinfo(const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	len = vsnprintf(infotext, INFOTEXTSIZE, fmt, args);
	va_end(args);

	if (len > INFOTEXTSIZE)
		len = INFOTEXTSIZE;
#ifdef SEMIHOSTING
	printf("%.*s", len, infotext);
#endif
	remote_tx(4, (uint8_t *)infotext, len);
}

struct tunestatus {
	uint16_t ant_U;
	uint16_t ant_I;
	uint8_t tuning_source;
	uint8_t realZ;
	uint8_t phi;
	uint8_t antenne;
	uint16_t c;
	uint16_t l;
	uint16_t iswr;
	uint16_t ipwr;
	uint32_t civ_freq;
	uint8_t civ_adr;
	uint8_t req_freq;
	uint8_t trx_relais;
	uint32_t tunedtofreq;
	uint8_t temp;
	uint16_t meas_UB;
} __attribute__((packed));

// sende Messwerte via serieller Schnittstelle
void send_values_serial()
{
	struct tunestatus status;

	if(free_fifo_size() < 3)
		return;

	memset(&status, 0, sizeof(status));

	// diese Daten nicht so oft, weil es zuviele sind
	// wenn die analogen Werte wackeln
	if(uart_tick > 0)
		return;

	uart_tick = 125;

	int iswr = (int)(fswr * 100);
	if(iswr > 65000)
		iswr = 65000;
	int ipwr = (int)(fwd_watt * 10);
	if(ipwr > 65000)
		ipwr = 65000;

	status.ant_U = htobe16(ant_U);
	status.ant_I = htobe16(ant_I);
	status.tuning_source = tuningsource;
	status.realZ = realZ;
	status.phi = phi;
	status.antenne = antenne;
	status.c = htobe16(calc_c());
	status.l = htobe16(calc_l());
	status.iswr = htobe16(iswr);
	status.ipwr = htobe16(ipwr);
	status.civ_freq = htobe32(getCIVfreq());
	status.civ_adr = eeconfig.civ_adr;
	status.req_freq = 0;
	status.trx_relais = relays[L_TRX].state;
	status.tunedtofreq = htobe32(tunedToFreq);
	status.temp = itemp;
	status.meas_UB = htobe16(meas_UB);
	// sende Memorybelegung
	uint8_t *pu = get_MemUsage();	// Datenlänge 10
	remote_tx(1, pu, 10);
	remote_tx(2, &status, sizeof(status));

	// Stellung der Relais (Länge 19)
	uint8_t relpos[RELAISANZAHL];
	memset(&relpos, 0, sizeof(relpos));
	for (int i = 0; i < RELAISANZAHL; i++) {
		if (relays[i].state)
			relpos[i] = 1;
	}
	remote_tx(3, (void *)relpos, RELAISANZAHL);
}

struct prot {
	uint16_t qrg;
	uint16_t swr;
	uint16_t l;
	uint8_t c;
};

void send_prot(uint16_t qrg, uint16_t swr, uint16_t L, uint8_t C)
{
	struct prot prot;

	memset(&prot, 0, sizeof(prot));

	prot.qrg = htobe16(qrg);
	prot.swr = htobe16(swr);
	prot.l = htobe16(L);
	prot.c = C;

	while(free_fifo_size() < 1)
		send_serial_fifo();

	remote_tx(0, &prot, sizeof(prot));
}

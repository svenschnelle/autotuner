/*
 * serial_tx_proc.c
 *
 *  Created on: 22.08.2018
 *      Author: kurt
 */

#include <main.h>

/*
 * zu sendende Datenblöcke max. 50 Byte lang
 * NICHT Thread safe ! Nur aus der Hauptschleife aufrufen, nie im IRQ
 */

// Fifo für zu sendende Datenblöcke
// die ersten beiden Bytes enthalten die Länge, LSB first
uint8_t txfifo[FIFO_DEPTH][FIFO_DATALEN + 2]; // zusätzlich 2 für die Länge
int wridx = 0, rdidx = 0;

// schreibe einen Datenblock in den fifo
int put_ser_tx_fifo(uint8_t *pdata, int len)
{
	// haben die Daten eine passende Länge ?
	if(len == 0 || len > (FIFO_DATALEN - 2))
		return 0;

	// ist im Fifo noch Platz ?
	if(((wridx+1) % FIFO_DEPTH) == rdidx)
		return 0;

	// Pointer auf den neuen Eintrag
	uint8_t *p = txfifo[wridx];

	// schreibe die Länge
	*p = len & 0xff;
	*(p+1) = (len >> 8) & 0xff;

	// schreibe Daten
	memcpy(p + 2, pdata, len);

	// stelle Schreibzeiger weiter
	if(++wridx == FIFO_DEPTH)
		wridx = 0;

	return 1;
}

// soviele Elemente passen in den Fifo
int free_fifo_size()
{
	int res;

	res = (FIFO_DEPTH - 1 + rdidx - wridx) % FIFO_DEPTH;

	return res;
}

// mit dieser Funtion können serielle Daten gesendet werden
// type: dient der Unterscheidung beim Empfänger:
// 0 ... Protokolldatensatz einer Tunerabstimmung
// 1 ... Memory Belegung
// 2 ... analoge Messwerte
// 3 ... Relaisstellungen
// 4 ... Infotext

#define MAXTYPES 10
uint16_t lastcrcPerType[MAXTYPES];

void helloFromGUI()
{
	for(int i=0; i < MAXTYPES; i++)
		lastcrcPerType[i] = 0;
}

void remote_tx(uint8_t type, void *pdata, int len)
{
	// prüfe die Länge, es kommen noch
	// 4x Header, 2xLänge, 2x CRC dazu
	// die Nutzdaten dürfen also FIFO_DATALEN-8 lang sein
	if(len > (FIFO_DATALEN-8))
		return;

	uint8_t txbuf[FIFO_DATALEN] = {0x32, 0x1c, 0xab};	// Header, das vierte ist der Typ
	txbuf[3] = type;

	txbuf[4] = (len >> 8) & 0xff;
	txbuf[5] = len & 0xff;

	memcpy(txbuf+6, pdata, len);

	uint16_t crc = crc16_messagecalc(txbuf, len + 6);
	txbuf[len + 6] = crc >> 8;
	txbuf[len + 7] = crc;

	if(crc != lastcrcPerType[type])
	{
		// nur bei Änderungen senden
		put_ser_tx_fifo(txbuf, len + 8);
		lastcrcPerType[type] = crc;
	}
}

// hole einen Eintrag aus dem Fifo
// return: Länge
int get_ser_tx_fifo(uint8_t *pdata)
{
	// sind Daten im Fifo ?
	if(rdidx == wridx)
		return 0;

	uint8_t *p = txfifo[rdidx];

	// Lese Länge
	int len = *(p+1);
	len <<= 8;
	len += *p;

	// prüfe die Länge
	if(len != 0 && len <= (FIFO_DATALEN - 2))
		memcpy(pdata, p+2, len);

	// stelle Lesezeiger weiter
	if(++rdidx == FIFO_DEPTH)
		rdidx = 0;

	return len;
}

// Aufruf aus der Hauptschleife
// sobald Daten im Fifo sind, sende diese aus
void send_serial_fifo()
{
uint8_t txdata[FIFO_DATALEN];

	if(remote_tx_ready() == 0)
		return;

	int len = get_ser_tx_fifo(txdata);
	if(len)
		remote_send(txdata, len);  // Sende seriell aus
}


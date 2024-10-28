/*
 * uart.c
 *
 *  Created on: 28.10.2017
 *      Author: kurt
 */

#include "main.h"

/*
 *
Controller fordert Frequenz an:
fe, fe, 00, e0, 03, fd

wenn im TRX der "Transceive" modus aktiviert ist,
dann sendet der TRX bei jedem Frequenzwechsel von selbst.

TRX antwortet:
fe, fe, e0, adr, 03, freq, fd

Controller Adress:
0xe0 ... Icom Default, auch vom DSP7 verwendet
0xe1 ... dieser Tuner

Codierung von freq:

5 bytes:

12 34 56 78 90 bedeutet:
1.234.567.890 Hz

es muss die Länge geprüft werden, da das IC735 nur
4 bytes sendet:
12 34 56 78 bedeutet
12.345.678 Hz

die Reihenfolge der Übertragung ist beginnend mit dem LSB,
in obigem Beispiel ist frequ:
90 78 56 34 12

 *
 */

void init_USART_DMA_TX_CIV(int len);
void handle_civrx(uint8_t data);
uint32_t bcdToint32(uint8_t *d, int mode);
uint8_t *byteToBCD(uint8_t v);

#define MAXCIVDATA 30
uint8_t civTXdata[MAXCIVDATA];
uint8_t civRXdata[MAXCIVDATA];
uint32_t civ_freq = 0;
uint8_t rxciv_adr;

/*
 * Überwache den Empfang von Frequenzmessages vom TRX:
 *
 * sollte eine Antwort für einen anderen Controller kommen 0xe0
 * so brauchen wir nicht selbst anzufragen und req_freq=0
 *
 * der Empfang von der freq wird überwacht mit got_civ_freq, welche
 * bei jedem Empfang auf 0 gesetzt wird.
 * Nach 2s (got_civ_freq==2000) wird die Eigenabfrage eingeschaltet.
 */
int req_freq = 0;

// ========== UART für das CI/V Interface ===============

// USART-1: PA9=TX, PA10=RX
// TX: DMA-2, Stream-7, Channel-4
// (RX: DMA-2, Stream-5, Channel-4), not used

void init_CIV_uart()
{
	// Enable USART2, DMA2 and GPIO clocks
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	// GPIO alternative function
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	// init UART-1
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = 4800;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1,&USART_InitStruct);

	/*
	USART_Cmd(USART1,ENABLE);
	while(1)
	{
		USART_SendData(USART1,0x55); // OK
	}*/

	// Init DMA
	init_USART_DMA_TX_CIV(MAXCIVDATA);

	// Init NVIC
	// DMA (TX) Transfer Complete IRQ
	NVIC_InitTypeDef   NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE); // DMA_IT_TC ... transfer complete

	// USART-1 IRQ
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART1,ENABLE);


	/*while(1)
	{
		civTXdata[0] = 0xfe;
		civTXdata[1] = 0xfe;
		civTXdata[2] = 0;
		civTXdata[3] = 0xe0;
		civTXdata[4] = 4;
		civTXdata[5] = 0xfd;
		civ_send(6);
		delay_1ms(1000);
	}*/
}

void init_USART_DMA_TX_CIV(int len)
{
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)civTXdata;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_BufferSize = len;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream7, &DMA_InitStructure);
}

void USART1_IRQHandler(void)
{
	if ((USART1->SR & USART_FLAG_RXNE) != (u16)RESET)
	{
		handle_civrx(USART_ReceiveData(USART1) & 0xff);

		USART_ClearFlag(USART1, USART_FLAG_RXNE);
	}

	if ((USART1->SR & USART_FLAG_ORE) != (u16)RESET)
	{
		// Receiver overrun
		// führt beim Debuggen zu ununterbrochenem Auslösen des IRQs
		USART_ClearFlag(USART1, USART_FLAG_ORE);
	}
}

void DMA2_Stream7_IRQHandler()
{
	if(DMA_GetITStatus(DMA2_Stream7,DMA_IT_TCIF7) == SET)
	{
		// mach irgendwas
		DMA_ClearITPendingBit(DMA2_Stream7,DMA_IT_TCIF7);
	}
}

// sende Datensatz via CIV
void civ_send_intern(uint8_t *pdata, int len)
{
	while(DMA_GetITStatus(DMA2_Stream7,DMA_IT_TCIF7) == SET);
		//return;	// letztes Senden noch nicht fertig

	for(int i=0; i<len; i++)
	{
		civTXdata[i] = pdata[i];
	}

	if(!DMA_GetCmdStatus(DMA2_Stream7))
	{
		init_USART_DMA_TX_CIV(len);
		DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
		DMA_Cmd(DMA2_Stream7, ENABLE);
		USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	}
}

uint64_t lastCIVsend = 0;

void civ_send(uint8_t *pdata, int len)
{
	if((tick_1ms - lastCIVsend) < 100)
	{
		delay_1ms(100);
	}

	civ_send_intern(pdata,len);
	lastCIVsend = tick_1ms;
}

void civ_request_frequency()
{
	if(civsendtick == 0 && req_freq == 1)
	{
		uint8_t cmd0_getfreq[6] = {0xfe, 0xfe, 0x00, 0xe0,    0x03, 0xfd};

		// CI-V 1 ist die Default Adresse, da nicht alle TRX die 00 als Broadcast erkennen
		cmd0_getfreq[3] = 0xE1; // Controller Address
		cmd0_getfreq[2] = eeconfig.civ_adr;

		civ_send(cmd0_getfreq, 6);
		civsendtick = 250;	// alle 500ms
	}
}

void cat_txrx(int txrx)
{
	uint8_t cmd_txrx[8] = {0xfe, 0xfe, 0x00, 0xe0,    0x1c, 0, 0, 0xfd};
	cmd_txrx[2] = eeconfig.civ_adr;

	if(txrx) cmd_txrx[6] = 1;

	civ_send(cmd_txrx, 8);
}

void cat_set_outpower(uint8_t pwr)
{
	uint8_t cmd_txrx[9] = {0xfe, 0xfe, 0x00, 0xe0,    0x14, 0x0a,  0,0,   0xfd};
	cmd_txrx[2] = eeconfig.civ_adr;

	uint8_t *p = byteToBCD(pwr);
	cmd_txrx[6] = p[0];
	cmd_txrx[7] = p[1];

	civ_send(cmd_txrx, 9);
}

void cat_mode(int mode)
{
	uint8_t cmd_txrx[7] = {0xfe, 0xfe, 0x00, 0xe0,    1, 1, 0xfd};
	cmd_txrx[2] = eeconfig.civ_adr;

	if(mode) cmd_txrx[5] = 5;

	civ_send(cmd_txrx, 7);
}

void cat_setqrg_Hz(uint32_t freq)
{
uint8_t farr[5];

	printinfo("~set QRG: %d kHz", freq/1000);

	uint32_t _10MHz = freq / 10000000;
	freq -= (_10MHz * 10000000);

	uint32_t _1MHz = freq / 1000000;
	freq -= (_1MHz * 1000000);

	uint32_t _100kHz = freq / 100000;
	freq -= (_100kHz * 100000);

	uint32_t _10kHz = freq / 10000;
	freq -= (_10kHz * 10000);

	uint32_t _1kHz = freq / 1000;
	freq -= (_1kHz * 1000);

	uint32_t _100Hz = freq / 100;
	freq -= (_100Hz * 100);

	uint32_t _10Hz = freq / 10;
	freq -= (_10Hz * 10);

	uint32_t _1Hz = freq;

	farr[0] = (uint8_t)((uint8_t)(_10Hz << 4) + (uint8_t)_1Hz);
	farr[1] = (uint8_t)((uint8_t)(_1kHz << 4) + (uint8_t)_100Hz);
	farr[2] = (uint8_t)((uint8_t)(_100kHz << 4) + (uint8_t)_10kHz);
	farr[3] = (uint8_t)((uint8_t)(_10MHz << 4) + (uint8_t)_1MHz);
	farr[4] = 0;

	cat_setqrg(farr);
}

void cat_setqrg(uint8_t *p)
{
	uint8_t cmd_txrx[11] = {0xfe, 0xfe, 0x00, 0xe0, 5,    0,0,0,0,0,   0xfd};
	cmd_txrx[2] = eeconfig.civ_adr;

	for(int i=0; i<5; i++)
		cmd_txrx[i+5] = p[i];

	civ_send(cmd_txrx, 11);
}

void cat_clearqrg(uint8_t *pfreq)
{
	// umdrehen
	uint8_t tmp = pfreq[0];
	pfreq[0] = pfreq[4];
	pfreq[4] = tmp;

	tmp = pfreq[1];
	pfreq[1] = pfreq[3];
	pfreq[3] = tmp;

	int ifreq = bcdToint32(pfreq,5);
	clearTuningValue(ifreq);
}

// wird aus dem RX irq aufgerufen
void handle_civrx(uint8_t data)
{
	// mache vorne Platz
	for(int i=(MAXCIVDATA-1); i>0; i--)
		civRXdata[i] = civRXdata[i-1];

	// neues Byte
	civRXdata[0] = data;

	// der Datensatz steht verkehrt herum in civRXdata
	if(civRXdata[0] == 0xfd)
	{
		// Ende erkannt
		// wenn die Antwort einem DSP-7 gilt (Controller Adr: 0xe0),
		// dann müssen wir nicht selbst anfragen
		if(civRXdata[8] == 0xe0 && civRXdata[9] == 0xfe && civRXdata[10] == 0xfe)
		{
			// 5 byte Datensatz
			rxciv_adr = civRXdata[7];
			civ_freq = bcdToint32(civRXdata+1,5);
			req_freq = 0;
			got_civ_freq = 0;
		}
		if(civRXdata[7] == 0xe0 && civRXdata[8] == 0xfe && civRXdata[9] == 0xfe)
		{
			// 4 byte Datensatz
			rxciv_adr = civRXdata[6];
			civ_freq = bcdToint32(civRXdata+1,4);
			req_freq = 0;
			got_civ_freq = 0;
		}

		// Antwort auf eine eigene Anfrage (Controller Adr: 0xe1)
		if(civRXdata[8] == 0xe1 && civRXdata[9] == 0xfe && civRXdata[10] == 0xfe)
		{
			// 5 byte Datensatz
			rxciv_adr = civRXdata[7];
			civ_freq = bcdToint32(civRXdata+1,5);
			got_civ_freq = 0;
		}
		if(civRXdata[7] == 0xe1 && civRXdata[8] == 0xfe && civRXdata[9] == 0xfe)
		{
			// 4 byte Datensatz
			rxciv_adr = civRXdata[6];
			civ_freq = bcdToint32(civRXdata+1,4);
			got_civ_freq = 0;
		}
	}
}

uint32_t bcdconv(uint8_t v, uint32_t mult)
{
uint32_t tmp,f;

	tmp = (v >> 4) & 0x0f;
	f = tmp * mult * 10;
	tmp = v & 0x0f;
	f += tmp * mult;

	return f;
}

// Wandle ICOM Frequenzangabe um
uint32_t bcdToint32(uint8_t *d, int mode)
{
uint32_t f=0;

	if(mode == 5)
	{
		f += bcdconv(d[0],100000000);
		f += bcdconv(d[1],1000000);
		f += bcdconv(d[2],10000);
		f += bcdconv(d[3],100);
		f += bcdconv(d[4],1);
	}

	if(mode == 4)
	{
		f += bcdconv(d[0],1000000);
		f += bcdconv(d[1],10000);
		f += bcdconv(d[2],100);
		f += bcdconv(d[3],1);
	}
	return f;
}

void makeBCD(uint32_t freq, uint8_t *farr)
{
	uint32_t _10MHz = freq / 10000000;
	freq -= (_10MHz * 10000000);

	uint32_t _1MHz = freq / 1000000;
	freq -= (_1MHz * 1000000);

	uint32_t _100kHz = freq / 100000;
	freq -= (_100kHz * 100000);

	uint32_t _10kHz = freq / 10000;
	freq -= (_10kHz * 10000);

	uint32_t _1kHz = freq / 1000;
	freq -= (_1kHz * 1000);

	uint32_t _100Hz = freq / 100;
	freq -= (_100Hz * 100);

	uint32_t _10Hz = freq / 10;
	freq -= (_10Hz * 10);

	uint32_t _1Hz = freq;

	farr[0] = (_10Hz << 4) + _1Hz;
	farr[1] = (_1kHz << 4) + _100Hz;
	farr[2] = (_100kHz << 4) + _10kHz;
	farr[3] = (_10MHz << 4) + _1MHz;
	farr[4] = 0;
}

uint8_t bcdbyte[2];
uint8_t *byteToBCD(uint8_t v)
{
	uint8_t s1 = v/100;
	v -= (s1*100);

	uint8_t s2 = v/10;
	v -= (s2*10);

	uint8_t s3 = v;

	bcdbyte[0] = s1;
	bcdbyte[1] = (s2 << 4) | s3;

	return bcdbyte;
}

uint32_t getCIVfreq()
{
uint32_t freq;

	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	freq = civ_freq;
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	// wenn 2s lang keine Antwort kam, schalte die Abfrage ein
	if(got_civ_freq > 2000)
		req_freq = 1;

	// wenn 5s lang Antwort kam, so ignoriere CIV Werte
	if(got_civ_freq > 5000)
		civ_freq = 0;

	return freq; // in Hz
}

void set_civ(uint8_t civ)
{
	eeconfig.civ_adr = civ;
}

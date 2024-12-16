/*
 * uart.c
 *
 *  Created on: 28.10.2017
 *      Author: kurt
 */

#include "main.h"

void init_USART_DMA_TX_CIV(int len);

#define MAX_TX_LEN 16
static uint8_t txbuf[MAX_TX_LEN];
static uint32_t civ_freq = 0;

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
	init_USART_DMA_TX_CIV(MAX_TX_LEN);

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
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)txbuf;
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

static uint32_t bcd2hex(uint8_t *data)
{
	return (((data[1] & 0xf) * 10) +
		((data[1] >> 4) * 100) +
		((data[0] & 0x0f) * 1000) +
		((data[0] >> 4) * 10000)) * 1000;
}

// sende Datensatz via CIV
void civ_send_intern(uint8_t *pdata, int len)
{
	while (DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) == SET);
		//return;	// letztes Senden noch nicht fertig

	for(int i=0; i < len; i++)
		txbuf[i] = pdata[i];

	if(!DMA_GetCmdStatus(DMA2_Stream7)) {
		init_USART_DMA_TX_CIV(len);
		DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
		DMA_Cmd(DMA2_Stream7, ENABLE);
		USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	}
}

static void handle_yaesu_cmd(uint8_t *data)
{
	uint8_t response[2];

	printinfo("Yaesu: %02x %02x %02x\n",
	       data[0], data[1], data[2]);
	switch (data[0]) {
	case 0xf0:
		/* Tuner enable */
		civ_freq = bcd2hex(data+1);
		response[0] = 0xa0;
		response[1] = 0xa1;
		civ_send_intern(response, 2);
		break;
	case 0xf1:
		/* Tuner disable */
		set_C_step(ANZ_C_STUFEN - 1);
		set_L_step(ANZ_L_STUFEN - 1);
		civ_freq = -1;
		response[1] = 0xa1;
		civ_send_intern(response, 2);
		break;
	case 0xf2:
		/* Start autotune */
		response[0] = 0xa0;
		civ_freq = bcd2hex(data+1);
		civ_send_intern(response, 1);
		seek_single();
		break;
	default:
		break;
	}
}

void uart_civ_tuning_done(int status)
{
	(void)status;
	uint8_t response = 0xa1;

	civ_send_intern(&response, sizeof(response));
}
// wird aus dem RX irq aufgerufen
static void handle_civrx(uint8_t data)
{
	static uint8_t cmd[3];
	static int index = 0;

	if (data == 0xff || index > 3) {
		index = 0;
		return;
	}
	cmd[index++] = data;
	if (index == 3) {
		handle_yaesu_cmd(cmd);
	}
}

void USART1_IRQHandler(void)
{
	if ((USART1->SR & USART_FLAG_RXNE) != (u16)RESET) {
		handle_civrx(USART_ReceiveData(USART1) & 0xff);
		USART_ClearFlag(USART1, USART_FLAG_RXNE);
	}

	if ((USART1->SR & USART_FLAG_ORE) != (u16)RESET) {
		// Receiver overrun
		// führt beim Debuggen zu ununterbrochenem Auslösen des IRQs
		USART_ClearFlag(USART1, USART_FLAG_ORE);
	}
}

void DMA2_Stream7_IRQHandler()
{
	if(DMA_GetITStatus(DMA2_Stream7,DMA_IT_TCIF7) == SET) {
		// mach irgendwas
		DMA_ClearITPendingBit(DMA2_Stream7,DMA_IT_TCIF7);
	}
}

void cat_txrx(int txrx)
{
	(void)txrx;
}

void cat_setqrg_Hz(uint32_t qrg)
{
	(void)qrg;
}

uint32_t getCIVfreq()
{
	return civ_freq;
}

/*
 * uart.c
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#include <main.h>

/*
 * Fernsteuerung via serieller Schnittstelle
 */

uint8_t uart_TX_data[MAXREMOTEDATA];

void init_uart()
{
	// Enable USART3, DMA1 (rx=Stream1-Ch4, tx=Stream3-Ch4) and GPIO clocks
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1 | RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	// GPIO alternative function
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);

	// init UART-3
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = 9600;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART3,&USART_InitStruct);

	// Init DMA
	init_USART3_DMA_TX(MAXREMOTEDATA);

	// Init NVIC
	// DMA (TX) Transfer Complete IRQ
	NVIC_InitTypeDef   NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	DMA_ITConfig(DMA1_Stream3, DMA_IT_TC, ENABLE); // DMA_IT_TC ... transfer complete

	// USART-3 IRQ
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART3,ENABLE);

}

void init_USART3_DMA_TX(int len)
{
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)uart_TX_data;
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
	DMA_Init(DMA1_Stream3, &DMA_InitStructure);
}

void USART3_IRQHandler(void)
{
	if ((USART3->SR & USART_FLAG_RXNE) != (u16)RESET)
	{
		handle_remoteData(USART_ReceiveData(USART3) & 0xff);

		USART_ClearFlag(USART3, USART_FLAG_RXNE);
	}

	if ((USART3->SR & USART_FLAG_ORE) != (u16)RESET)
	{
		// Receiver overrun
		// führt beim Debuggen zu ununterbrochenem Auslösen des IRQs
		USART_ClearFlag(USART3, USART_FLAG_ORE);
	}
}

int uart_dma_running = 0;

// TX ready
void DMA1_Stream3_IRQHandler()
{
	if(DMA_GetITStatus(DMA1_Stream3,DMA_IT_TCIF3) == SET)
	{
		uart_dma_running = 0;
		DMA_ClearITPendingBit(DMA1_Stream3,DMA_IT_TCIF3);
	}
	/*if(DMA_GetITStatus(DMA1_Stream3,DMA_IT_HTIF3) == SET)
	{
		DMA_ClearITPendingBit(DMA1_Stream3,DMA_IT_HTIF3);
	}
	if(DMA_GetITStatus(DMA1_Stream3,DMA_IT_TEIF3) == SET)
	{
		DMA_ClearITPendingBit(DMA1_Stream3,DMA_IT_TEIF3);
	}
	if(DMA_GetITStatus(DMA1_Stream3,DMA_IT_DMEIF3) == SET)
	{
		DMA_ClearITPendingBit(DMA1_Stream3,DMA_IT_DMEIF3);
	}
	if(DMA_GetITStatus(DMA1_Stream3,DMA_IT_FEIF3) == SET)
	{
		DMA_ClearITPendingBit(DMA1_Stream3,DMA_IT_FEIF3);
	}*/
}

// ist TX fertig ?
int remote_tx_ready()
{
	if(uart_dma_running)
		return 0;	// letztes Senden ist noch nicht fertig

	return 1;
}

// sende Datensatz
int remote_send(uint8_t *pdata, int len)
{
	if(uart_dma_running)
		return 0;	// letztes Senden ist noch nicht fertig

	for(int i=0; i<len; i++)
		uart_TX_data[i] = pdata[i];

	if(!DMA_GetCmdStatus(DMA1_Stream3))
	{
		uart_dma_running = 1;
		init_USART3_DMA_TX(len);
		DMA_ITConfig(DMA1_Stream3, DMA_IT_TC, ENABLE);
		DMA_Cmd(DMA1_Stream3, ENABLE);
		USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
	}
	return 1;
}

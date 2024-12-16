/*
 * uart.h
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#ifndef UART_H_
#define UART_H_

void init_uart();
void init_USART3_DMA_TX(int len);
int remote_send(uint8_t *pdata, int len);
void cat_setqrg(uint8_t *p);
int remote_tx_ready();

#endif /* UART_H_ */

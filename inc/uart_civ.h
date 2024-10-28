/*
 * uart_civ.h
 *
 *  Created on: 12.05.2018
 *      Author: kurt
 */

#ifndef UART_CIV_H_
#define UART_CIV_H_

void init_CIV_uart();
uint32_t getCIVfreq();
void civ_request_frequency();
void cat_txrx(int txrx);
void cat_mode(int mode);
void makeBCD(uint32_t freq, uint8_t *farr);
uint32_t bcdToint32(uint8_t *d, int mode);
void cat_clearqrg(uint8_t *pfreq);
void cat_setqrg_Hz(uint32_t qrg);
void set_civ(uint8_t civ);
void cat_set_outpower(uint8_t pwr);

extern uint32_t civ_freq;
extern int req_freq;

#endif /* UART_CIV_H_ */

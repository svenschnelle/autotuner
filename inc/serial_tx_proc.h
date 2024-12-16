/*
 * serial_tx_proc.h
 *
 *  Created on: 22.08.2018
 *      Author: kurt
 */

#ifndef SERIAL_TX_PROC_H_
#define SERIAL_TX_PROC_H_

#define FIFO_DATALEN	50
#define FIFO_DEPTH		20

void send_serial_fifo();
void remote_tx(uint8_t type, void *pdata, int len);
int free_fifo_size();
void helloFromGUI();

#endif /* SERIAL_TX_PROC_H_ */

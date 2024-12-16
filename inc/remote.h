/*
 * remote.h
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#ifndef REMOTE_H_
#define REMOTE_H_

void handle_remoteData(uint8_t data);
void execute_Remote();
void send_values_serial();
void printinfo(const char *fmt, ...);
void send_prot(uint16_t qrg, uint16_t swr, uint16_t L, uint8_t C);

#endif /* REMOTE_H_ */

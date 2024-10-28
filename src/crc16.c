/*
 * crc16.c
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#include <main.h>

// ====== CRC Funktionen ======

 uint16_t reg16 = 0xffff;         // Schieberegister

 uint16_t crc16_bytecalc(unsigned char byte)
 {
	 int i;
	 uint16_t polynom = 0x8408;        // Generatorpolynom

	 for (i=0; i<8; ++i)
	 {
		 if ((reg16&1) != (byte&1))
            reg16 = (uint16_t)(reg16>>1)^polynom;
		 else
            reg16 >>= 1;
		 byte >>= 1;
	 }
	 return reg16;             // inverses Ergebnis, MSB zuerst
 }

 uint16_t crc16_messagecalc(unsigned char *data, int len)
 {
	 int i;

	 reg16 = 0xffff;					// Initialisiere Shift-Register mit Startwert
	 for(i=0; i<len; i++) {
		 reg16 = crc16_bytecalc(data[i]);		// Berechne fuer jeweils 8 Bit der Nachricht
	 }
	 return reg16;
 }


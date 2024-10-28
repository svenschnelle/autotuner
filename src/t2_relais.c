/*
 * t2_relais.c
 *
 *  Created on: 07.04.2018
 *      Author: kurt
 */

/*
 * Bedienung der bistabilen Relais
 * Pulszeit zum Schalten min. 25ms
 * diese sind wie folgt angeschlossen:
 *
 * Kondensatoren:
 *
 * C=3200pF: aus=STEPPER1=PC0, ein=STEPPER3=PC2
 * C=1600pF: aus=REL31=RELAIS30=PE14, ein=REL32=RELAIS31=PE15
 * C= 800pF: aus=REL29=RELAIS26=PE10, ein=REL30=RELAIS27=PE11
 * C= 400pF: aus=REL27=RELAIS24=PE8 , ein=REL28=RELAIS25=PE9
 * C= 200pF: aus=REL25=RELAIS22=PE6 , ein=REL26=RELAIS23=PE7
 * C= 100pF: aus=REL23=RELAIS20=PE4,  ein=REL24=RELAIS21=PE5
 * C=  50pF: aus=REL21=RELAIS18=PE2,  ein=REL22=RELAIS19=PE3
 * C=  25pF: aus=REL19=RELAIS16=PE0,  ein=REL20=RELAIS17=PE1
 * (unbenutzte Stepperports auf 0-Output halten)
 *
 * L auf TRX Seite:
 * aus=STEPPER5=PC4, ein=STEPPER7=PC6
 *
 * L auf ANT Seite:
 * aus=STEPPER6=PC5, ein=STEPPER8=PC7
 *
 * Spulen:
 *
 * L=  50nH: aus=REL17=RELAIS14=PD14, ein=REL18=RELAIS15=PD15
 * L= 100nH: aus=REL1=RELAIS0=PD0, ein=REL2=RELAIS1=PD1
 * L= 200nH: aus=REL3=RELAIS2=PD2, ein=REL4=RELAIS3=PD3
 * L= 400nH: aus=REL5=RELAIS4=PD4, ein=REL6=RELAIS5=PD5
 * L= 800nH: aus=REL7=RELAIS6=PD6, ein=REL8=RELAIS7=PD7
 * L= 1,6uH: aus=REL9=RELAIS8=PD8, ein=REL10=RELAIS9=PD9
 * L= 3,2uH: aus=REL11=RELAIS28=PE12, ein=REL12=RELAIS29=PE13
 * L= 6,4uH: aus=REL13=RELAIS10=PD10, ein=REL14=RELAIS11=PD11
 * L=12,8uH: aus=REL15=RELAIS12=PD12, ein=REL16=RELAIS13=PD13
 *
 * Eingeschaltet werden die Relais per Funktion
 * Ausgeschaltet nach x ms von einem Timer
 */

#include <main.h>

void do_t2relais(int relais, int coilnr, int onoff);

int relstat_ON[RELAISANZAHL];
int relstat_OFF[RELAISANZAHL];
uint8_t relpos[RELAISANZAHL];

GPIO_TypeDef *rel_gpios[RELAISANZAHL] =
{
	GPIOC,
	GPIOC,

	GPIOE,
	GPIOE,
	GPIOE,
	GPIOE,
	GPIOE,
	GPIOE,
	GPIOE,
	GPIOC,

	GPIOD,
	GPIOD,
	GPIOD,
	GPIOD,
	GPIOD,
	GPIOD,
	GPIOE,
	GPIOD,
	GPIOD,
};

int relON_ports[RELAISANZAHL] =
{
	1<<5,
	1<<4,

	1<<1,
	1<<3,
	1<<5,
	1<<7,
	1<<9,
	1<<11,
	1<<15,
	1<<0,

	1<<15,
	1<<1,
	1<<3,
	1<<5,
	1<<7,
	1<<9,
	1<<13,
	1<<11,
	1<<13,
};

int relOFF_ports[RELAISANZAHL] =
{
	1<<7,
	1<<6,

	1<<0,
	1<<2,
	1<<4,
	1<<6,
	1<<8,
	1<<10,
	1<<14,
	1<<2,

	1<<14,
	1<<0,
	1<<2,
	1<<4,
	1<<6,
	1<<8,
	1<<12,
	1<<10,
	1<<12,
};

void init_t2relais()
{
	// schalte die GPIO Takte ein
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	// schalte die GPIOs auf Output
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin=0xFFFF; // alle Ports 0 bis 15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;

	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin=0x7FFF; // Ports 0 bis 14
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// lege alle auf 0 (inaktiv)
	GPIO_ResetBits(GPIOD, 0xFFFF);
	GPIO_ResetBits(GPIOE, 0xFFFF);
	GPIO_ResetBits(GPIOC, 0x7FFF);

	for(int i=0; i<RELAISANZAHL; i++)
		relpos[i] = 255;
}

// diese Funktion ruft man auf um ein Relais zu schalten
void switch_t2relais(int relais, int onoff)
{
	if(relais >= RELAISANZAHL) return;

	if(onoff == TOGGLE)
	{
		if(relpos[relais] == 1) onoff = 0;
		if(relpos[relais] == 0) onoff = 1;
	}

	if(onoff)
	{
		if(relpos[relais] != 1)
		{
			relstat_ON[relais] = RELHOLDTIME;
			relpos[relais] = 1;
		}
	}
	else
	{
		if(relpos[relais] != 0)
		{
			relstat_OFF[relais] = RELHOLDTIME;
			relpos[relais] = 0;
		}
	}
}

// wird NUR aus dem Timer aufgerufen, alle 1ms
// Startet und beendet einen Relaisimpuls
void reset_t2relais()
{
	for(int i=0; i<RELAISANZAHL; i++)
	{
		if(relstat_ON[i])
		{
			if(relstat_ON[i] == RELHOLDTIME)
				do_t2relais(i,ON,ON);		// Beginne einen Relaisimpulse

			if(relstat_ON[i] == 1)
				do_t2relais(i,ON,OFF);	// Zeit abgelaufen, beende Relaisimpulse

			relstat_ON[i]--;
		}

		if(relstat_OFF[i])
		{
			if(relstat_OFF[i] == RELHOLDTIME)
				do_t2relais(i,OFF,ON);		// Beginne einen Relaisimpulse

			if(relstat_OFF[i] == 1)
				do_t2relais(i,OFF,OFF);	// Zeit abgelaufen, beende Relaisimpulse

			relstat_OFF[i]--;
		}
	}
}

// 1 wenn irgendein Relais gerade geschaltet wird
int isRelaisActive()
{
	for(int i=0; i<RELAISANZAHL; i++)
	{
		if(relstat_ON[i])
			return 1;

		if(relstat_OFF[i])
			return 1;
	}

	return 0;
}

// aktiviere/deaktiviere die Relaisspulen
void do_t2relais(int relais, int coilnr, int onoff)
{
	if(onoff == ON)
	{
		if(coilnr == ON)
			GPIO_SetBits(rel_gpios[relais], relON_ports[relais]);
		else
			GPIO_SetBits(rel_gpios[relais], relOFF_ports[relais]);
	}
	else
	{
		if(coilnr == ON)
			GPIO_ResetBits(rel_gpios[relais], relON_ports[relais]);
		else
			GPIO_ResetBits(rel_gpios[relais], relOFF_ports[relais]);
	}
}


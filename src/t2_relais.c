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

struct relay relays[RELAISANZAHL] = {
	[L_TRX]  = { .gpio = GPIOC, .on = 5,  .off = 7  },
	[L_ANT]  = { .gpio = GPIOC, .on = 4,  .off = 6  },
	[C25]    = { .gpio = GPIOE, .on = 1,  .off = 0  },
	[C50]    = { .gpio = GPIOE, .on = 3,  .off = 2  },
	[C100]   = { .gpio = GPIOE, .on = 5,  .off = 4  },
	[C200]   = { .gpio = GPIOE, .on = 7,  .off = 6  },
	[C400]   = { .gpio = GPIOE, .on = 9,  .off = 8  },
	[C800]   = { .gpio = GPIOE, .on = 11, .off = 10 },
	[C1600]  = { .gpio = GPIOE, .on = 15, .off = 14 },
	[C3200]  = { .gpio = GPIOC, .on = 0,  .off = 2  },
	[L50]    = { .gpio = GPIOD, .on = 15, .off = 14 },
	[L100]   = { .gpio = GPIOD, .on = 1,  .off = 0  },
	[L200]   = { .gpio = GPIOD, .on = 3,  .off = 2  },
	[L400]   = { .gpio = GPIOD, .on = 5,  .off = 4  },
	[L800]   = { .gpio = GPIOD, .on = 7,  .off = 6  },
	[L1600]  = { .gpio = GPIOD, .on = 9,  .off = 8  },
	[L3200]  = { .gpio = GPIOE, .on = 13, .off = 12 },
	[L6400]  = { .gpio = GPIOD, .on = 11, .off = 10 },
	[L12800] = { .gpio = GPIOD, .on = 13, .off = 12 },
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

	for(int i=0; i < RELAISANZAHL; i++)
		relays[i].state = 255;
}

// diese Funktion ruft man auf um ein Relais zu schalten
void switch_t2relais(int num, int onoff)
{
	if(num >= RELAISANZAHL)
		return;

	struct relay *r = relays + num;

	if(onoff == TOGGLE)
		onoff = r->state ? 0 : 1;

	if(onoff) {
		if(r->state != 1) {
			r->timer_on = RELHOLDTIME;
			r->state = 1;
		}
	} else {
		if(r->state != 0) {
			r->timer_off = RELHOLDTIME;
			r->state = 0;
		}
	}
}


// aktiviere/deaktiviere die Relaisspulen
static void do_t2relais(int num, int coilnr, int onoff)
{
	struct relay *r = relays + num;

	if(onoff == ON) {
		if(coilnr == ON)
			GPIO_SetBits(r->gpio, 1 << r->on);
		else
			GPIO_SetBits(r->gpio, 1 << r->off);
	} else {
		if(coilnr == ON)
			GPIO_ResetBits(r->gpio, 1 << r->on);
		else
			GPIO_ResetBits(r->gpio, 1 << r->off);
	}
}


// wird NUR aus dem Timer aufgerufen, alle 1ms
// Startet und beendet einen Relaisimpuls
void reset_t2relais(void)
{
	for(int i = 0; i < RELAISANZAHL; i++) {
		struct relay *r = relays + i;
		if(r->timer_on) {
			if(r->timer_on == RELHOLDTIME)
				do_t2relais(i, ON, ON);		// Beginne einen Relaisimpulse

			if(r->timer_on == 1)
				do_t2relais(i, ON, OFF);	// Zeit abgelaufen, beende Relaisimpulse

			r->timer_on--;
		}

		if(r->timer_off) {
			if(r->timer_off == RELHOLDTIME)
				do_t2relais(i, OFF, ON);	// Beginne einen Relaisimpulse

			if(r->timer_off == 1)
				do_t2relais(i, OFF, OFF);	// Zeit abgelaufen, beende Relaisimpulse

			r->timer_off--;
		}
	}
}

// 1 wenn irgendein Relais gerade geschaltet wird
int isRelaisActive()
{
	for(int i = 0; i < RELAISANZAHL; i++) {
		struct relay *r = relays + i;
		if(r->timer_on || r->timer_off)
			return 1;
	}

	return 0;
}

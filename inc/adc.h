/*
 * adc.h
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#ifndef ADC_H_
#define ADC_H_

void t2_ADC_Init();
int ui16_Read_ADC1_ConvertedValue(int channel);
int scan_analog_inputs();
int isAdcConvReady();
int getRev();
int cf_calc_temp(unsigned long uin);

#define ADC_DR_OFFSET	0x4c

extern int revvoltage, fwdvoltage;
extern int phi;		// Phi: 0=+, 1=-
extern int z;		// Z  : 0= >50, 1= <50
extern int realZ;
extern int ant_U,ant_I;
extern int meas_u, meas_i;
extern int itemp,meas_UB;

#endif /* ADC_H_ */

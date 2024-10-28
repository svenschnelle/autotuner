/*
 * swr.c
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#include <main.h>

/*
 * calculate SWR
 * SWR = (Vf+Vr)/(Vf-Vr)
 *
 * fwd und rev sind Spannungen welche den dBm Wert entsprechen
 * die Skalierung ist von der PwrSwr Platine abhängig
 * für einen standard Aufbau können folgende Richtwerte benutzt werden:
 * 3 W ... 1332 mV
 * 700 W ... 1787 mV
 */

float stepmV_per_dB_fwd;			// Millivolt pro dB
float stepmV_per_dB_rev;			// Millivolt pro dB

// Ergebnisse
float fwd_watt = 0;
float rev_watt = 0;
float fswr;

float WtoDBM(float watts)
{
	return 10.0*log10(watts*1000.0);
}

void cal_3watt()
{
	eeconfig.refmVlow_rev = eeconfig.refmVlow_fwd = fwdvoltage;
	eeconfig.refU3W = meas_u;
	eeconfig.refI3W = meas_i;
	init_swr();
}

void cal_100watt()
{
	eeconfig.refmVhigh_fwd = eeconfig.refmVhigh_rev = fwdvoltage;
	eeconfig.refU100W = meas_u;
	eeconfig.refI100W = meas_i;
	init_swr();
}

// berechne Konstanten
void init_swr()
{
	eeconfig.ref_low_dBm = WtoDBM(eeconfig.refWlow);
	eeconfig.ref_high_dBm = WtoDBM(eeconfig.refWhigh);
	stepmV_per_dB_fwd = (eeconfig.refmVhigh_fwd - eeconfig.refmVlow_fwd)/(eeconfig.ref_high_dBm - eeconfig.ref_low_dBm);
	stepmV_per_dB_rev = (eeconfig.refmVhigh_rev - eeconfig.refmVlow_rev)/(eeconfig.ref_high_dBm - eeconfig.ref_low_dBm);
}

void calc_swr(int fwd, int rev)
{
	float fwdvolt = fwd;
	float revvolt = rev;

	// calculate forward
	 float variation =  fwdvolt - eeconfig.refmVlow_fwd;			// difference in volts to xwatts reference
	 variation =  variation/stepmV_per_dB_fwd;			// difference in dB to xwatts reference
	 float fwd_dBm =  variation + eeconfig.ref_low_dBm;			// real measured dBm value
	 // calculate real power in W
	 fwd_watt = pow(10,fwd_dBm/10.0) / 1000;
	 if(fwd_watt > 9999) fwd_watt = 9999;
	 float Vf = sqrt(fwd_watt*50*1000);

	 // calculate reverse
	 variation =  revvolt - eeconfig.refmVlow_rev;			// difference in volts to xwatts reference
	 variation =  variation/stepmV_per_dB_rev;			// difference in dB to xwatts reference
	 float rev_dBm =  variation + eeconfig.ref_low_dBm;		// real measured dBm value
	 // calculate real power in W
	 rev_watt = pow(10,rev_dBm/10.0) / 1000;
	 if(rev_watt > 9999) rev_watt = 9999;
	 float Vr = sqrt(rev_watt*50*1000);

	 // calculate swr
	 if(Vf == Vr || Vf < Vr) fswr = 99;
	 else fswr = (Vf+Vr)/(Vf-Vr);

	 if(fswr<1.0001) fswr = 1.0001;
}

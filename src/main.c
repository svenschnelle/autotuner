/*
	Display 7
	Version 1.0
	19.9.2017
	Kurt Moraw
	DJ0ABR
*/

#include <main.h>

uint32_t last_freq = 0;
uint8_t last_antenne = 99;

int main(void)
{
	init_t2led();
	init_t2relais();
	TIM3_Initialization();	// 1ms Tick für die Relaisrückstellung
	init_uart();
	init_CIV_uart();
	t2_ADC_Init();
	init_eeprom();
	init_storage();
	init_swr();
	set_t2led(GREENCOL);

	while(1)
	{
		execute_Remote();
		scan_analog_inputs();
		send_values_serial();
		send_serial_fifo();
		civ_request_frequency();
		tune();
		tune_full();
		tune_fullband();

		uint32_t currf = getCIVfreq();
		if(currf != 0 && !no_tuneFromMem)
		{
			if((abs((int32_t)currf - (int32_t)last_freq) >= 10000) ||
			   antenne != last_antenne)
			{
				// die CIV Frequenz hat sich um mehr als 10kHz geändert
				// stelle den Tuner aus dem Memory ein
				tune_fromMem();
				last_freq = currf;
				last_antenne = antenne;
			}
		}

		if(ee_savetimer == 0)
		{
			checkConfig();
			copyMemToEE();
			ee_savetimer = EESAVETIME;
		}
	}
}

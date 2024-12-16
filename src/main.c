/*
	Display 7
	Version 1.0
	19.9.2017
	Kurt Moraw
	DJ0ABR
*/

#include <main.h>
#include <stdio.h>
#include <lcd.h>
#include <i2c.h>

int main(void)
{
	uint8_t last_antenne = 99;
	uint32_t last_freq = 0;

#ifdef SEMIHOSTING
	void initialise_monitor_handles(void);
	initialise_monitor_handles();
#endif
	TIM3_Initialization();	// 1ms Tick für die Relaisrückstellung
	init_t2led();
	init_t2relais();
	i2c_init();
#ifdef WITH_LCD
	lcd_init();
#endif
	init_uart();
	init_CIV_uart();
	t2_ADC_Init();
	init_eeprom();
	init_storage();
	init_swr();

	set_t2led(GREENCOL);

	while(1) {
		delay_1ms(10);
		execute_Remote();
		scan_analog_inputs();
		send_values_serial();
		send_serial_fifo();
		tune();
//		tune_fullband();

		uint32_t currf = getCIVfreq();
		if(currf != 0 && !no_tuneFromMem) {
			if((abs((int32_t)currf - (int32_t)last_freq) >= 10000) ||
			   antenne != last_antenne) {
				// die CIV Frequenz hat sich um mehr als 10kHz geändert
				// stelle den Tuner aus dem Memory ein
				tune_fromMem();
				last_freq = currf;
				last_antenne = antenne;
			}
		}

		if(ee_savetimer == 0) {
			checkConfig();
			copyMemToEE();
			ee_savetimer = EESAVETIME;
		}
#ifdef WITH_LCD
		if (lcd_timer == 0) {
			lcd_printf(0, 0, "F%7.1fW  R%7.1fW", fwd_watt, rev_watt);
			lcd_printf(1, 0, "C%6.2fnF  L%6.2fuH", (float)calc_c() / 1000, (float)calc_l() / 1000);
			lcd_printf(2, 7, "QRG %6.2fMHz", (float)getCIVfreq() / 1000000.0);
			lcd_printf(3, 11, "SWR %5.2f", fswr);
			lcd_timer = LCDTIME;
		}
#endif
	}
}

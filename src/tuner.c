/*
 * tuner.c
 *
 *  Created on: 11.08.2018
 *      Author: kurt
 */

#include <main.h>

int grundstellung();
int seek50_plusphi_lower_l(int init);
int seek_phi0_lower_c(int init);
int seek_adjL(int init);
int seek_lowest_swr(int init);
int seek_phineg_lowerC(int init);
int seek_z50_lowerC(int init);
int seek_phiplus_lowerL(int init);
int storeBest();
void setBest();
void resetBest();
void stop_tuning();
void insert_prot();

int tuningspeed = 25;	// ms, Schrittgeschwindigkeit des Autotuners
						// die Relaiszeit ist 20ms, so het er noch 5ms Reserve für ADC usw.
int do_tuning = 0;		// 1 wenn ein neues Tunerintervall beginnt
int smithpos = 0;		// 0=normal, 1=oberes Abstimmfeld
float suchval = 1.1;	// zu suchendes SWR
int seek_complete = 0;
int tuningsource = 0;	// getuned=0 oder vom Memory=1
int bestL, bestC;
float bestswr;
int tune_band = 0;
uint32_t tune_freq;
uint32_t stopfreq;
int no_tuneFromMem = 0;
int tuning_single = 0;
int tuning_band = 0;
int lastproc = NO_TUNING;
int best_swr_found = 0;
int old_tuning_band = 0;

char procnames[MAXTUNINGPROCS][30] =
{
		"idle",
		"Grundstellung",
		"Suche 50, P+, verkl.L",
		"Suche P-, verkl. C",
		"Scan kleinstes SWR",
		"Suche min SWR, adj.L",
		"Suche P-, verkl.C, korr.Z",
		"Suche 50, verkl.C",
		"Suche P+, verkl. L",
		"alt: SeekComplete"
};

// wird laufend aus der Mainloop aufgerufen
// führt einen Tuningschritt durch wenn der Timer abgelaufen ist
// und zwar vom gewählten Tunerprozess, solange bis dieser beendet ist
int act_tuner_process = NO_TUNING;

void tune()
{
int init = 0;
int ret = 0;

	// Timer Intervallsteuerung
	// keine Aktivität solange Relais geschalten werden
	if(!do_tuning || isRelaisActive()) return;
	do_tuning = 0;

	// prüfe ob gesendet wird
	if(fswr == 0)
	{
		act_tuner_process = NO_TUNING;
		tuning_single = 0;
		tuning_band = 0;
		old_tuning_band = 0;
		lastproc = NO_TUNING;
		return;
	}

	// wenn ein neuer Prozess gewählt wurde,
	// so sage ihm, dass er sich initialisieren kann
	if(act_tuner_process != lastproc)
	{
		init = 1;
		lastproc = act_tuner_process;
		printinfo("%d: %s",act_tuner_process,procnames[act_tuner_process]);
	}

	// prüfe ob gewünschtes SWR erreicht ist
	if(act_tuner_process != NO_TUNING && storeBest())
	{
		// falls das gewünschte SWR erreicht ist, beende den Prozess
		printinfo("gefunden SWR: %.2f",fswr);
		saveTuningValue();
		act_tuner_process = NO_TUNING;
		best_swr_found = 1;
	}

	// springe die Prozesse an
	// wenn ein Prozess fertig ist, so gibt er 0 zurück
	switch (act_tuner_process)
	{
	case GRUNDSTELLUNG:
		ret = grundstellung();
		break;
	case SEEK50_PLUSPHI_LOWER_L:
		ret = seek50_plusphi_lower_l(init);
		break;
	case SEEK_PHINEG_LOWER_C:
		ret = seek_phi0_lower_c(init);
		break;
	case SEEK_ADJ_L:
		seek_adjL(init);
		break;
	case SEEK_LOWEST_SWR:
		ret = seek_lowest_swr(init);
		break;
	case SEEK_PHI_NEG_LOWER_C:
		ret = seek_phineg_lowerC(init);
		break;
	case SEEK_Z50_LOWER_C:
		ret = seek_z50_lowerC(init);
		break;
	case SEEK_PHIPLUS_LOWERL:
		ret = seek_phiplus_lowerL(init);
		break;
	}

	if(ret == 0)
	{
		// der Prozess hat FERTIG gemeldet
		act_tuner_process = NO_TUNING;
	}
}

// Anforderung einer einzelnen Tuning-Aktion vom GUI aus
void requestTuning(int nr)
{
	if(fswr == 0)
	{
		printinfo("Bitte TX einschalten");
		return;
	}
	resetBest();
	act_tuner_process = nr;
}

// steuert den Ablauf vom Komplett-Tunings
float lastfswr = 9999;
void tune_full()
{
static int old_tuning_single = 0;
static int started_proc = NO_TUNING;

	if(tuning_single)
	{
		// komplettes Tuning nur für die aktuelle Frequenz
		if(old_tuning_single == 0)
		{
			// das Single Tuning wurde soeben eingeschaltet
			// Initialisiere
			old_tuning_single = 1;
			if(fswr == 0)
			{
				printinfo("Bitte TX einschalten");
				stop_tuning();
				return;
			}
			printinfo("Starte Einzel-TUNING");
			resetBest();
			// wenn das vorherige SWR recht gut ist, mache nur einen SWR-Scan
			if(lastfswr <= (suchval + 0.3))
				started_proc = act_tuner_process = SEEK_LOWEST_SWR;
			else
				started_proc = act_tuner_process = GRUNDSTELLUNG;
		}

		if(best_swr_found)
		{
			// das gewünschte swr wurde gefunden, beende
			best_swr_found = 0;
			started_proc = NO_TUNING;
			lastproc = NO_TUNING;
			if(!tuning_band)
				cat_txrx(0);
			printinfo("Scan FERTIG");
			insert_prot();
			tuning_single = 0;
			old_tuning_single = 0;
			lastfswr = bestswr;
			return;
		}

		if(started_proc != act_tuner_process && act_tuner_process == NO_TUNING)
		{
			// der aktuelle Tunerprozess hat sich beendet, gehe zum nächsten
			//printinfo("%d: %s fertig",started_proc,procnames[started_proc]);
			switch(started_proc)
			{
			case GRUNDSTELLUNG:
				if(smithpos == 0)
					started_proc = act_tuner_process = SEEK50_PLUSPHI_LOWER_L;
				else
					started_proc = act_tuner_process = SEEK_PHI_NEG_LOWER_C;
				break;

			// Smith Fall rechts unten
			case SEEK50_PLUSPHI_LOWER_L:
				started_proc = act_tuner_process = SEEK_PHINEG_LOWER_C;
				break;
			case SEEK_PHINEG_LOWER_C:
				// Feinabstimmung
				started_proc = act_tuner_process = SEEK_LOWEST_SWR;
				break;

			// Smith Fall links oben
			case SEEK_PHI_NEG_LOWER_C:
				started_proc = act_tuner_process = SEEK_Z50_LOWER_C;
				break;
			case SEEK_Z50_LOWER_C:
				started_proc = act_tuner_process = SEEK_PHIPLUS_LOWERL;
				break;
			case SEEK_PHIPLUS_LOWERL:
				// Feinabstimmung
				started_proc = act_tuner_process = SEEK_LOWEST_SWR;
				break;

			case SEEK_LOWEST_SWR:
				// alles fertig
				started_proc = NO_TUNING;
				act_tuner_process = NO_TUNING;
				lastproc = NO_TUNING;
				if(!tuning_band)
					cat_txrx(0);
				printinfo("Scan FERTIG");
				saveTuningValue();
				insert_prot();
				tuning_single = 0;
				old_tuning_single = 0;
				lastfswr = bestswr;
				break;
			}
		}
	}
}

// steuert das Tuning eines ganzen Bandes
void tune_fullband()
{
static uint32_t startqrg, stopqrg, actqrg;
static uint32_t wait_qrg = 0;

	if(tuning_band)
	{
		if(wait_qrg)
		{
			// eine freq wurde im TRX eingestellt, warte bis diese bestätigt wird
			// dann starte das Scannen
			if(wait_qrg == civ_freq)
			{
				tuning_single = 1;
				wait_qrg = 0;
			}
			else
			{
				// die QRG stimmt noch nicht
				// warte einen Moment und fordere sie nochmal an
				delay_1ms(50);
				cat_setqrg_Hz(actqrg);
			}
		}

		if(old_tuning_band == 0)
		{
			// das Band Tuning wurde soeben eingeschaltet
			// Initialisiere
			lastfswr = 9999;
			old_tuning_band = 1;
			if(fswr == 0)
			{
				printinfo("Bitte TX einschalten");
				stop_tuning();
				return;
			}
			printinfo("Starte Band-TUNING");
			no_tuneFromMem = 1;
			getFreqEdges(civ_freq , &startqrg, &stopqrg);
			// stelle die erste qrg ein
			actqrg = startqrg;
			cat_setqrg_Hz(actqrg);
			wait_qrg = actqrg;
		}

		if(tuning_single == 0 && wait_qrg == 0)
		{
			// die Frequenz ist fertig, gehe zur nächsten
			actqrg += 10000;
			if(actqrg > stopqrg)
			{
				// fertig
				cat_txrx(0);
				printinfo("Bandscan FERTIG");
				tuning_band = 0;
				no_tuneFromMem = 0;
				old_tuning_band = 0;
				return;

			}
			cat_setqrg_Hz(actqrg);
			wait_qrg = actqrg;
		}
	}
}

void resetBest()
{
	bestswr = 9999;
	bestC = ANZ_C_STUFEN - 1;
	bestL = ANZ_L_STUFEN - 1;
}

// alle Ls und Cs eingeschaltet, L an ANT oder TRX falls noch nicht gewählt
int grundstellung()
{
	// schalte alle Cs und Ls ein
	for(int i=C25; i<=C3200; i++)
		switch_t2relais(i, ON);

	for(int i=L50; i<=L12800; i++)
		switch_t2relais(i, OFF);

	// Schalte L auf ANT order TRX
	if((!relpos[L_TRX] && !relpos[L_ANT])||(relpos[L_TRX] && relpos[L_ANT]))
	{
		if(z <= 50 && phi > 0)
		{
			switch_t2relais(L_TRX, 1);
			switch_t2relais(L_ANT, 0);
			smithpos = 1;
		}
		else
		{
			switch_t2relais(L_TRX, 0);
			switch_t2relais(L_ANT, 1);
			smithpos = 0;
		}
	}

	if(relpos[L_ANT]) smithpos = 0;
	else smithpos = 1;

	return 0;	// fertig
}

// Suche Z=50 und PHI>0 durch verkleinern des Ls
int seek50_plusphi_lower_l(int init)
{
static int fein = 0;
static int last_lstep;
int lstep;

	if(init)
	{
		fein = 0;
	}

	// prüfe ob fertig
	if(z <= 50 && phi > 0)
	{
		if(!fein)
		{
			// gehe einen Schritt zurück und stelle auf Feinsuche
			lstep = last_lstep;
			fein = 1;
		}
		else
			return 0;
	}
	else
	{
		// lese aktuelle L-Stellung
		lstep = store_actL();
		last_lstep = lstep;
	}

	if(fein)
		lstep--;
	else
	{
		int dist = lstep/2;
		if(dist <= 0) dist = 1;
		lstep -= dist;
	}

	// wenn lstep=0 ist, so sind alle Spulen weggeschaltet, weniger geht nich
	// Reduziere jetzt das C solange bis es doch geht
	if(lstep < 0)
	{
		int cstep = store_actC();

		if(cstep == 0)
		{
			// alle Cs und alle Ls sind weg, gebe auf
			return 1;
		}

		if(cstep == 1)
			cstep = 0;
		else
			cstep /= 2;
		set_C_step(cstep);

		lstep = ANZ_L_STUFEN-1;
		last_lstep = lstep;
		fein = 0;
	}

	set_L_step(lstep);

	return 1;
}

// sucht Phi < 0 durch Verkleinern des Cs
// prüft und korrigiert L damit es auf der Z50 Linie bleibt
int seek_phi0_lower_c(int init)
{
static int fein = 0;
static int last_cstep;
static int phase = 0;	// 0=stelle C, 1=korr.L
static int dist;
static int lstep;
static int lastZ;

	if(init)
	{
		fein = 0;
		phase = 0;
	}

	if(z == 50)
		phase = 0;  // keine L Einstellung notwendig

	// prüfe ob fertig
	if(phi < 0)
	{
		if(fein == 0)
		{
			// gehe einen Schritt zurück und stelle auf Feinsuche-1
			set_C_step(last_cstep);
			fein = 1;
			phase = 0;
			return 1;
		}
		if(fein == 1)
		{
			// gehe einen Schritt zurück und stelle auf Feinsuche-2
			set_C_step(last_cstep);
			fein = 2;
			phase = 0;
			return 1;
		}
		else
		{
			// wenn phi<0 ist, so muss aber auch Z im Fenster sein
			// damit wir wirklich fertig sind
			int al = store_actL();
			if(z  != 50 && al && al < 511)
			{
				phase = 1;
				lastZ = z;
			}
			else
			{
				// fertig
				setBest();
				return 0;
			}
		}
	}

	if(phase == 0)
	{
		// Stelle Cs ein
		int cstep = store_actC();	// aktuell geschaltete Cs
		last_cstep = cstep;

		if(fein == 2)
			cstep--;
		else if(fein == 1)
		{
			cstep-=2;
		}
		else
		{
			dist = cstep/2;
			if(dist <= 0) dist = 1;
			cstep -= dist;
			cstep -= 1;
		}

		set_C_step(cstep);

		// wenn cstep=0 ist, so sind alle Cs geschaltet, mehr geht nicht, beende
		// auch ein Feinabgleich wird nicht mehr benötigt
		if(cstep == 0)
		{
			setBest();
			return 0;
		}

		dist = 1;
		lastZ = z;
		phase = 1;	// als nächstes suche die Z=50 Linie
	}
	else
	{
		// wenn das Z das Fenster überspringt, so sind wir
		// auch auf der Linie
		if((lastZ < 50 && z > 50) || (lastZ > 50 && z < 50))
		{
			lastZ = z = 50;
		}
		else
			lastZ = z;

		// suche die Z=50 Linie
		if(lastZ == 50)
		{
			// wir sind auf der Linie
			phase = 0;	// mache wieder mit C weiter
			return 1;
		}

		// wenn z<50, dann L erhöhen, sonst L verkleinern
		lstep = store_actL();
		if(lastZ > 50)
		{
			if(lstep <= 0)
			{
				// alle Ls sind weg, weniger geht nicht, beenden
				setBest();
				return 0;
			}
			set_L_step(lstep - 1);
		}
		else
		{
			if(lstep >= 511)
			{
				// alle Ls sind da, mehr geht nicht, beenden
				setBest();
				return 0;
			}
			set_L_step(lstep + 1);
		}
	}

	return 1;
}

// ändere L für kleinstes SWR
int seek_adjL(int init)
{
static int Lstep, dir;
static int actSWR;
int iswr;

	iswr = (int)(fswr * 10);

	if(init)
	{
		Lstep = store_actL();

		// ermittle Stellrichtung
		dir = 0;

		// test L+1
		scan_analog_inputs();
		float swr = fswr;
		set_L_step(Lstep + 1);
		delay_1ms(50);
		scan_analog_inputs();
		if(fswr < swr)
			dir = 1;

		// zurückstellen
		set_L_step(Lstep);
		delay_1ms(50);
		scan_analog_inputs();
		swr = fswr;

		// teste L-1
		set_L_step(Lstep - 1);
		delay_1ms(50);
		scan_analog_inputs();
		if(fswr < swr)
			dir = -1;

		if(dir == 0)
		{
			// es ändert sich nichts
			set_L_step(Lstep);
			return 0;
		}

		actSWR = iswr;
	}
	else
	{
		if(iswr > actSWR)
		{
			// wir sind übers Minimum hinausgegangen, stelle zurück und beende
			setBest();
			return 0;
		}
	}

	actSWR = iswr;

	Lstep += dir;
	set_L_step(Lstep);

	return 1;
}

void set_suchVal(int v)
{
	suchval = v;
	suchval /= 100;

	if(suchval < 1.05) suchval = 1.05;
}

void set_suchSpeed(int v)
{
	tuningspeed = v;
}

int seek_phineg_lowerC(int init)
{
static int fein;
static int last_cstep;

	if(init)
		fein = 0;

	// prüfe ob fertig
	if(phi < 0)
	{
		if(fein == 0)
		{
			// gehe einen Schritt zurück und stelle auf Feinsuche
			set_C_step(last_cstep);
			fein = 1;
			do_tuning = 1;	// mache sofort weiter
			return 1;
		}
		else
		{
			return 0;
		}
	}

	// Stelle Cs ein
	int cstep = store_actC();	// aktuell geschaltete Cs
	last_cstep = cstep;

	if(fein == 1)
		cstep--;
	else
		cstep /= 2;

	set_C_step(cstep);

	// wenn cstep=0 ist, so sind alle Cs geschaltet, mehr geht nicht, beende
	// auch ein Feinabgleich wird nicht mehr benötigt
	if(cstep == 0)
		return 0;

	return 1;
}

int seek_z50_lowerC(int init)
{
static int fein;
static int last_cstep;
static int lastz;

	if(init)
	{
		fein = 0;
		lastz = z;
		last_cstep = store_actC();
	}

	// prüfe ob fertig
	if((realZ>=45 && realZ<=55) || (lastz<=50 && z>50) || (lastz>=50 && z<50))
	{
		if(fein == 0)
		{
			// gehe einen Schritt zurück und stelle auf Feinsuche
			set_C_step(last_cstep);
			fein = 1;
			return 1;
		}
		else
			return 0;
	}

	lastz = z;

	// Stelle Cs ein
	int cstep = store_actC();	// aktuell geschaltete Cs
	last_cstep = cstep;

	if(fein == 1)
		cstep--;
	else
		cstep /= 2;

	set_C_step(cstep);

	// wenn cstep=0 ist, so sind alle Cs geschaltet, mehr geht nicht, beende
	// auch ein Feinabgleich wird nicht mehr benötigt
	if(cstep == 0)
		return 0;

	return 1;
}

int seek_phiplus_lowerL(int init)
{
static int fein = 0;
static int last_lstep;
static int phase = 0;	// 0=stelle L, 1=korr.C
static int dist;
static int cstep;
static int lastZ;

	if(init)
	{
		fein = 0;
		phase = 0;
	}

	if(z == 50)
		phase = 0;  // keine L Einstellung notwendig

	// prüfe ob fertig
	if(phi > 0)
	{
		if(fein == 0)
		{
			// gehe einen Schritt zurück und stelle auf Feinsuche
			set_L_step(last_lstep);
			fein = 1;
			phase = 0;
			return 1;
		}
		if(fein == 1)
		{
			// gehe einen Schritt zurück und stelle auf Feinsuche
			set_L_step(last_lstep);
			fein = 2;
			phase = 0;
			return 1;
		}
		else
		{
			// wenn phi>0 ist, so muss aber auch Z im Fenster sein
			// damit wir wirklich fertig sind
			if(z  != 50 && store_actC())
			{
				phase = 1;
				lastZ = z;
			}
			else
			{
				// fertig
				return 0;
			}
		}
	}

	if(phase == 0)
	{
		// Stelle Ls ein
		int lstep = store_actL();	// aktuell geschaltete Ls
		last_lstep = lstep;

		if(fein == 2)
			lstep--;
		else if(fein == 1)
			lstep-=2;
		else
		{
			dist = lstep/2;
			if(dist <= 0) dist = 1;
			lstep -= dist;
			lstep -= 1;
		}

		set_L_step(lstep);

		// wenn lstep=0 ist, so sind alle Ls geschaltet, mehr geht nicht, beende
		// auch ein Feinabgleich wird nicht mehr benötigt
		if(lstep == 0)
			return 0;

		dist = 1;
		lastZ = z;
		phase = 1;	// als nächstes suche die Z=50 Linie
	}
	else
	{
		// wenn das Z das Fenster überspringt, so sind wir
		// auch auf der Linie
		if((lastZ < 50 && z > 50) || (lastZ > 50 && z < 50))
			z = 50;

		lastZ = z;

		// suche die Z=50 Linie
		if(z == 50)
		{
			// wir sind auf der Linie
			phase = 0;	// mache wieder mit C weiter
			return 1;
		}

		// wenn z>50, dann C erhöhen, sonst L verkleinern
		cstep = store_actC();
		if(z < 50)
		{
			if(cstep <= 0)
			{
				// alle Cs sind weg, weniger geht nicht, beenden
				return 0;
			}
			set_C_step(cstep - 1);
		}
		else
		{
			if(cstep >= 255)
			{
				// alle Cs sind da, mehr geht nicht, beenden
				return 0;
			}
			set_C_step(cstep + 1);
		}
	}

	return 1;
}

int storeBest()
{
	if(fswr < bestswr)
	{
		bestswr = fswr;
		bestL = store_actL();
		bestC = store_actC();
	}

	if(bestswr <= suchval)
	{
		// beende sofort wenn gewünschtes SWR gefunden wurde
		return 1;
	}
	return 0;
}

void setBest()
{
	set_L_step(bestL);
	set_C_step(bestC);
}

// scanne einen Quadranten
int seek_quadrant(int init, int qnum, int startL, int startC)
{
static float lswrL, lswrC;
static int C, L;
int maxdistL = 128;
int maxdistC = 16;
int ret = 1;

	if(init)
	{
		L = startL;
		C = startC;
		lswrL = lswrC = 9999;
		return 1;
	}

	// gehe vom Startpunkt aus das C nach oben/unten
	// für jedes C scanne die L-Linie vom Startpunkt nach oben/unten
	// sobald keine Verbesserung des SWRs mehr ist, beende
	int setNewC = 0;

	// prüfe ob die Veränderung des Ls was gebracht hat
	if(fswr > (lswrL + 0.1))
	{
		// es wird deutlich schlechter, beende dieses L, gehe zum nächsten C
		L = startL;
		setNewC = 1;
	}
	else
	{
		if(fswr < lswrL)
			lswrL = fswr;	// neuer Beststand

		// gehe zum nächsten L
		if(qnum == 0 || qnum == 2)
		{
			L++;
			if(L >= (startL+maxdistL) || L > ANZ_L_STUFEN)
			{
				// L ist fertig durchlaufen, gehe zum nächsten C
				L = startL;
				setNewC = 1;
			}
		}
		else
		{
			L--;
			if(L <= (startL-maxdistL) || L < 0)
			{
				// L ist fertig durchlaufen, gehe zum nächsten C
				L = startL;
				setNewC = 1;
			}
		}
	}

	if(setNewC)
	{
		// stelle das nächste C ein

		// prüfe ob der letzte L-Linien Scan ein besseres swr gebracht hat als der vorherige
		if(lswrL > (lswrC + 0.1))
		{
			// es wurde schlechter, wir können aufhören
			ret = 0;
		}
		else
		{
			if(lswrL < lswrC)
				lswrC = lswrL;	// neuer Linie-Bestand

			// gehe zum nächsten C
			lswrL = 9999;
			if(qnum == 0 || qnum == 1)
			{
				C++;
				if(C >= (startC+maxdistC) || C > ANZ_C_STUFEN)
				{
					// C ist fertig durchlaufen, beende
					ret = 0;
				}
			}
			else
			{
				C--;
				if(C <= (startC-maxdistC) || C < 0)
				{
					// C ist fertig durchlaufen, beende
					ret = 0;
				}
			}
		}
	}

	if(ret == 1)
	{
		// setze neue Relaisposition
		set_C_step(C);
		set_L_step(L);
	}
	else
	{
		set_C_step(startC);
		set_L_step(startL);
	}

	return ret;
}

int seek_lowest_swr(int init)
{
static int startL, startC;
static int quadr_num;

	if(init)
	{
		// Mittelpunkt (=Startpunkt) der Suche, der Prozess zuvor muss sein gefundenes Best eingestellt haben
		startL = store_actL();
		startC = store_actC();

		quadr_num = 0;
		seek_quadrant(1, quadr_num, startL, startC);
		return 1;
	}

	// die Suche findet in 4 Quadranten statt:
	// C+/L+ , C+/L- , C-/L+ , C-/L-
	// jeder Quadrant beendet, wenn er keine Verbesserung mehr hat und
	// sprint zum nächsten Quadranten

	if(seek_quadrant(0, quadr_num, startL, startC) == 0)
	{
		if(++quadr_num > 3)
		{
			setBest();
			return 0;
		}
		// Initialisiere den neuen Quadranten
		seek_quadrant(1, quadr_num, startL, startC);
	}

	return 1;
}

// scanne die aktuell eingestellte QRG
void seek_single()
{
	if(civ_freq == 0) return;

	cat_txrx(1);
	delay_1ms(500);

	tuning_single = 1;
}

// Tune das komplette aktuelle Band in 10kHz Schritten
void seek_band()
{
	if(civ_freq == 0) return;

	cat_txrx(1);
	delay_1ms(500);

	tuning_band = 1;
}

void stop_tuning()
{
	act_tuner_process = NO_TUNING;
	tuning_single = 0;
	tuning_band = 0;
	old_tuning_band = 0;
	lastproc = NO_TUNING;
	cat_txrx(0);
	printinfo("Band-Scan STOP");
}

// Protokollierung der Scans
void insert_prot()
{
	uint16_t qrgprotocol = (uint16_t)(civ_freq / 1000);
	uint16_t swrprotocol = (uint16_t)(bestswr * 100);
	uint16_t Lprotocol = store_actL();
	uint8_t Cprotocol = store_actC();

	send_prot(qrgprotocol, swrprotocol, Lprotocol, Cprotocol);
}

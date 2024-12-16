/*
 * tuner.c
 *
 *  Created on: 11.08.2018
 *      Author: kurt
 */

#include <main.h>
#include <stdbool.h>

unsigned int tuningspeed = 25;	// ms, Schrittgeschwindigkeit des Autotuners
				// die Relaiszeit ist 20ms, so het er noch 5ms Reserve für ADC usw.
int do_tuning = 0;		// 1 wenn ein neues Tunerintervall beginnt
int tuningsource;	// getuned=0 oder vom Memory=1
int no_tuneFromMem = 0;
int tuning_band;
int old_tuning_band;

static int smithpos;		// 0=normal, 1=oberes Abstimmfeld
static float suchval = 1.1;	// zu suchendes SWR
static int bestL, bestC;
static float bestswr;
static int tuning_single;
static int lastproc = NO_TUNING;
static int best_swr_found;

static const char *procnames[] =
{
	[NO_TUNING] = "NO_TUNING",
	[START_TUNING] = "START_TUNING",
	[GRUNDSTELLUNG] = "GRUNDSTELLUNG",
	[SEEK50_PLUSPHI_LOWER_L] = "SEEK50_PLUSPHI_LOWER_L",
	[SEEK_PHINEG_LOWER_C] = "SEEK_PHINEG_LOWER_C",
	[SEEK_LOWEST_SWR] = "SEEK_LOWEST_SWR",
	[SEEK_PHI_NEG_LOWER_C] = "SEEK_PHI_NEG_LOWER_C",
	[SEEK_Z50_LOWER_C] = "SEEK_Z50_LOWER_C",
	[SEEK_PHIPLUS_LOWERL] = "SEEK_PHIPLUS_LOWERL",
	[TUNING_DONE] = "TUNING_DONE",
};

// alle Ls und Cs eingeschaltet, L an ANT oder TRX falls noch nicht gewählt
static int grundstellung(int init)
{
	if (init) {
		// schalte alle Cs und Ls ein
		for (int i = C25; i <= C3200; i++)
			switch_t2relais(i, ON);

		for (int i = L50; i <= L12800; i++)
			switch_t2relais(i, OFF);

		switch_t2relais(L_TRX, 0);
		switch_t2relais(L_ANT, 0);
		return 1;
	}

	// Schalte L auf ANT order TRX
	if (realZ <= 50 && phi > 0) {
		switch_t2relais(L_TRX, 1);
		switch_t2relais(L_ANT, 0);
		smithpos = 1;
	} else {
		switch_t2relais(L_TRX, 0);
		switch_t2relais(L_ANT, 1);
		smithpos = 0;
	}

	if (relays[L_ANT].state)
		smithpos = 0;
	else
		smithpos = 1;

	return 0;	// fertig
}

// Suche Z=50 und PHI>0 durch verkleinern des Ls
static int seek50_plusphi_lower_l(int init)
{
	static int fein = 0;
	static int last_lstep;
	int lstep;

	if(init)
		fein = 0;

	// prüfe ob fertig
	if(z <= 50 && phi > 0) {
		if(!fein) {
			// gehe einen Schritt zurück und stelle auf Feinsuche
			lstep = last_lstep;
			fein = 1;
		} else {
			return 0;
		}
	} else {
		// lese aktuelle L-Stellung
		lstep = store_actL();
		last_lstep = lstep;
	}

	if(fein) {
		lstep--;
	} else {
		int dist = lstep / 2;
		if(dist <= 0)
			dist = 1;
		lstep -= dist;
	}

	// wenn lstep=0 ist, so sind alle Spulen weggeschaltet, weniger geht nich
	// Reduziere jetzt das C solange bis es doch geht
	if(lstep < 0) {
		int cstep = store_actC();

		if(cstep == 0) {
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

static void setBest()
{
	set_L_step(bestL);
	set_C_step(bestC);
}

static void resetBest()
{
	bestswr = 9999;
	bestC = ANZ_C_STUFEN - 1;
	bestL = ANZ_L_STUFEN - 1;
}

static int storeBest()
{
	if(fswr < bestswr) {
		bestswr = fswr;
		bestL = store_actL();
		bestC = store_actC();
	}

	if(bestswr <= suchval) {
		// beende sofort wenn gewünschtes SWR gefunden wurde
		return 1;
	}
	return 0;
}

// sucht Phi < 0 durch Verkleinern des Cs
// prüft und korrigiert L damit es auf der Z50 Linie bleibt
static int seek_phi0_lower_c(int init)
{
	static int fein = 0;
	static int last_cstep;
	static int phase = 0;	// 0=stelle C, 1=korr.L
	static int dist;
	static int lstep;
	static int lastZ;

	if(init) {
		fein = 0;
		phase = 0;
	}

	if(z == 50)
		phase = 0;  // keine L Einstellung notwendig

	// prüfe ob fertig
	if(phi < 0) {
		if(fein == 0) {
			// gehe einen Schritt zurück und stelle auf Feinsuche-1
			set_C_step(last_cstep);
			fein = 1;
			phase = 0;
			return 1;
		}
		if(fein == 1) {
			// gehe einen Schritt zurück und stelle auf Feinsuche-2
			set_C_step(last_cstep);
			fein = 2;
			phase = 0;
			return 1;
		} else {
			// wenn phi < 0 ist, so muss aber auch Z im Fenster sein
			// damit wir wirklich fertig sind
			int al = store_actL();
			if(z  != 50 && al && al < 511) {
				phase = 1;
				lastZ = z;
			} else {
				// fertig
				setBest();
				return 0;
			}
		}
	}

	if(phase == 0) {
		// Stelle Cs ein
		int cstep = store_actC();	// aktuell geschaltete Cs
		last_cstep = cstep;

		if(fein == 2)
			cstep--;
		else if(fein == 1) {
			cstep -= 2;
		} else {
			dist = cstep / 2;
			if(dist <= 0)
				dist = 1;
			cstep -= dist;
			cstep -= 1;
		}

		if (cstep < 0) {
			setBest();
			return 0;
		}
		set_C_step(cstep);

		// wenn cstep=0 ist, so sind alle Cs geschaltet, mehr geht nicht, beende
		// auch ein Feinabgleich wird nicht mehr benötigt
		if(cstep == 0) {
			setBest();
			return 0;
		}

		dist = 1;
		lastZ = z;
		phase = 1;	// als nächstes suche die Z=50 Linie
	} else {
		// wenn das Z das Fenster überspringt, so sind wir
		// auch auf der Linie
		if((lastZ < 50 && z > 50) || (lastZ > 50 && z < 50)) {
			lastZ = z = 50;
		} else {
			lastZ = z;
		}
		// suche die Z=50 Linie
		if(lastZ == 50) {
			// wir sind auf der Linie
			phase = 0;	// mache wieder mit C weiter
			return 1;
		}

		// wenn z < 50, dann L erhöhen, sonst L verkleinern
		lstep = store_actL();
		if(lastZ > 50) {
			if(lstep <= 0) {
				// alle Ls sind weg, weniger geht nicht, beenden
				setBest();
				return 0;
			}
			set_L_step(lstep - 1);
		} else {
			if(lstep >= 511) {
				// alle Ls sind da, mehr geht nicht, beenden
				setBest();
				return 0;
			}
			set_L_step(lstep + 1);
		}
	}
	return 1;
}

// scanne einen Quadranten
static int seek_quadrant(int init, int qnum, int startL, int startC)
{
	static float lswrL, lswrC;
	static int C, L;
	int maxdistL = 128;
	int maxdistC = 16;
	int ret = 1;

	printinfo("Q %d, init %d C%d L%d SWR %5.3f phi %d Z %d\n", qnum, init, startL, startC, fswr, phi, realZ);
	if(init) {
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
	if(fswr > (lswrL + 0.02)) {
		// es wird deutlich schlechter, beende dieses L, gehe zum nächsten C
		L = startL;
		setNewC = 1;
	} else {
		if(fswr < lswrL)
			lswrL = fswr;	// neuer Beststand

		// gehe zum nächsten L
		if(qnum == 0 || qnum == 2) {
			L++;
			if(L >= (startL+maxdistL) || L > ANZ_L_STUFEN) {
				// L ist fertig durchlaufen, gehe zum nächsten C
				L = startL;
				setNewC = 1;
			}
		} else {
			L--;
			if(L <= (startL-maxdistL) || L < 0) {
				// L ist fertig durchlaufen, gehe zum nächsten C
				L = startL;
				setNewC = 1;
			}
		}
	}

	if(setNewC) {
		// stelle das nächste C ein

		// prüfe ob der letzte L-Linien Scan ein besseres swr gebracht hat als der vorherige
		if(lswrL > (lswrC + 0.02)) {
			// es wurde schlechter, wir können aufhören
			ret = 0;
		} else {
			if(lswrL < lswrC)
				lswrC = lswrL;	// neuer Linie-Bestand

			// gehe zum nächsten C
			lswrL = 9999;
			if(qnum == 0 || qnum == 1) {
				C++;
				if(C >= (startC + maxdistC) || C >= ANZ_C_STUFEN) {
					// C ist fertig durchlaufen, beende
					ret = 0;
				}
			} else {
				C--;
				if(C <= (startC-maxdistC) || C < 0)
				{
					// C ist fertig durchlaufen, beende
					ret = 0;
				}
			}
		}
	}

	if(ret == 1) {
		// setze neue Relaisposition
		set_C_step(C);
		set_L_step(L);
	} else {
		set_C_step(startC);
		set_L_step(startL);
	}

	return ret;
}

static int seek_lowest_swr(int init)
{
	static int startL, startC;
	static int quadr_num;

	if(init) {
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

	if(seek_quadrant(0, quadr_num, startL, startC) == 0) {
		if(++quadr_num > 3) {
			setBest();
			return 0;
		}
		// Initialisiere den neuen Quadranten
		seek_quadrant(1, quadr_num, startL, startC);
	}

	return 1;
}

static int seek_phineg_lowerC(int init)
{
	static int fein;
	static int last_cstep;

	if(init)
		fein = 0;

	// prüfe ob fertig
	if(phi < 0) {
		if(fein == 0) {
			// gehe einen Schritt zurück und stelle auf Feinsuche
			set_C_step(last_cstep);
			fein = 1;
			do_tuning = 1;	// mache sofort weiter
			return 1;
		} else {
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

static int seek_z50_lowerC(int init)
{
	static int fein;
	static int last_cstep;
	static int lastz;

	if(init) {
		fein = 0;
		lastz = z;
		last_cstep = store_actC();
	}

	// prüfe ob fertig
	if((realZ>=45 && realZ<=55) || (lastz<=50 && z>50) || (lastz>=50 && z<50)) {
		if(fein == 0) {
			// gehe einen Schritt zurück und stelle auf Feinsuche
			set_C_step(last_cstep);
			fein = 1;
			return 1;
		} else {
			return 0;
		}
	}

	lastz = z;

	// Stelle Cs ein
	int cstep = store_actC();	// aktuell geschaltete Cs
	last_cstep = cstep;

	if(fein == 1)
		cstep--;
	else
		cstep /= 2;

	if (cstep < 0 || cstep > ANZ_C_STUFEN)
		return 0;
	set_C_step(cstep);

	// wenn cstep=0 ist, so sind alle Cs geschaltet, mehr geht nicht, beende
	// auch ein Feinabgleich wird nicht mehr benötigt
	if(cstep == 0)
		return 0;

	return 1;
}

static int seek_phiplus_lowerL(int init)
{
	static int fein = 0;
	static int last_lstep;
	static int phase = 0;	// 0=stelle L, 1=korr.C
	static int dist;
	static int cstep;
	static int lastZ;

	if(init) {
		fein = 0;
		phase = 0;
	}

	if(z == 50)
		phase = 0;  // keine L Einstellung notwendig

	// prüfe ob fertig
	if(phi > 0) {
		if(fein == 0) {
			// gehe einen Schritt zurück und stelle auf Feinsuche
			set_L_step(last_lstep);
			fein = 1;
			phase = 0;
			return 1;
		}

		if(fein == 1) {
			// gehe einen Schritt zurück und stelle auf Feinsuche
			set_L_step(last_lstep);
			fein = 2;
			phase = 0;
			return 1;
		} else {
			// wenn phi>0 ist, so muss aber auch Z im Fenster sein
			// damit wir wirklich fertig sind
			if(z  != 50 && store_actC()) {
				phase = 1;
				lastZ = z;
			} else {
				// fertig
				return 0;
			}
		}
	}

	if(phase == 0) {
		// Stelle Ls ein
		int lstep = store_actL();	// aktuell geschaltete Ls
		last_lstep = lstep;

		if(fein == 2) {
			lstep--;
		} else if(fein == 1) {
			lstep-=2;
		} else {
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
	} else {
		// wenn das Z das Fenster überspringt, so sind wir
		// auch auf der Linie
		if((lastZ < 50 && z > 50) || (lastZ > 50 && z < 50))
			z = 50;

		lastZ = z;

		// suche die Z=50 Linie
		if(z == 50) {
			// wir sind auf der Linie
			phase = 0;	// mache wieder mit C weiter
			return 1;
		}

		// wenn z>50, dann C erhöhen, sonst L verkleinern
		cstep = store_actC();
		if(z < 50) {
			if(cstep <= 0) {
				// alle Cs sind weg, weniger geht nicht, beenden
				return 0;
			}
			set_C_step(cstep - 1);
		} else {
			if(cstep >= 255) {
				// alle Cs sind da, mehr geht nicht, beenden
				return 0;
			}
			set_C_step(cstep + 1);
		}
	}
	return 1;
}

static tuning_state_t old_tuning_state = NO_TUNING, tuning_state = NO_TUNING;
static bool tuning_init;
// steuert den Ablauf vom Komplett-Tunings
float lastfswr = 9999;

static void tuning_new_state(tuning_state_t state)
{
	tuning_state = state;
	tuning_init = state != old_tuning_state;
	if (tuning_init)
		printinfo("%s -> %s\n", procnames[old_tuning_state], procnames[state]);
	old_tuning_state = tuning_state;
}


// Protokollierung der Scans
static void insert_prot()
{
	uint32_t civ_freq = getCIVfreq();
	uint16_t qrgprotocol = (uint16_t)(civ_freq / 1000);
	uint16_t swrprotocol = (uint16_t)(bestswr * 100);
	uint16_t Lprotocol = store_actL();
	uint8_t Cprotocol = store_actC();

	send_prot(qrgprotocol, swrprotocol, Lprotocol, Cprotocol);
}

void tune(void)
{
	static int retry;
	int init = tuning_init;

	// Timer Intervallsteuerung
	// keine Aktivität solange Relais geschalten werden
	if(isRelaisActive())
		return;

	tuning_init = 0;
	if (tuning_state != NO_TUNING)
		printinfo("%s(%s) SWR %.2f\n", procnames[tuning_state], init ? "init" : "", fswr);

	// prüfe ob gewünschtes SWR erreicht ist
	switch (tuning_state) {
	case NO_TUNING:
	case START_TUNING:
	case GRUNDSTELLUNG:
	case TUNING_DONE:
		break;
	default:
		if (fwd_watt < 1) {
			printinfo("Kein Traeger vorhanden\n");
			tuning_new_state(NO_TUNING);
			break;
		}

		if (storeBest()) {
			// falls das gewünschte SWR erreicht ist, beende den Prozess
			printinfo("SWR gefunden: %.2f (required %.2f)\n", fswr, bestswr);
			tuning_new_state(TUNING_DONE);
			best_swr_found = 1;
			break;
		}
	}

	// springe die Prozesse an
	// wenn ein Prozess fertig ist, so gibt er 0 zurück
	switch (tuning_state) {
	case NO_TUNING:
		if(tuning_single) {
			printinfo("Starte Einzel-TUNING\n");
			tuning_new_state(START_TUNING);
			tuning_single = 0;
		}
		break;

	case START_TUNING:
		if(fwd_watt < 1 && retry < 100) {
			delay_1ms(10);
			break;
		}
		if (retry == 100) {
			printinfo("Bitte TX einschalten\n");
			tuning_new_state(TUNING_DONE);
			break;
		}
		resetBest();

		// wenn das vorherige SWR recht gut ist, mache nur einen SWR-Scan
//		if(lastfswr <= (suchval + 0.3))
//			tuning_new_state(SEEK_LOWEST_SWR);
//		else
			tuning_new_state(GRUNDSTELLUNG);
//		break;

	case GRUNDSTELLUNG:
		if (!grundstellung(init)) {
			if(smithpos == 0)
				tuning_new_state(SEEK50_PLUSPHI_LOWER_L);
			else
				tuning_new_state(SEEK_PHI_NEG_LOWER_C);
		}
		break;
	case SEEK50_PLUSPHI_LOWER_L:
		if (!seek50_plusphi_lower_l(init))
			tuning_new_state(SEEK_PHINEG_LOWER_C);
		break;
	case SEEK_PHINEG_LOWER_C:
		if (!seek_phi0_lower_c(init))
			tuning_new_state(SEEK_LOWEST_SWR);
		break;
	case SEEK_PHI_NEG_LOWER_C:
		if (!seek_phineg_lowerC(init))
			tuning_new_state(SEEK_Z50_LOWER_C);
		break;
	case SEEK_Z50_LOWER_C:
		if (!seek_z50_lowerC(init))
			tuning_new_state(SEEK_PHIPLUS_LOWERL);
		break;
	case SEEK_PHIPLUS_LOWERL:
		if (!seek_phiplus_lowerL(init))
			tuning_new_state(SEEK_LOWEST_SWR);
		break;
	case SEEK_LOWEST_SWR:
		if (!seek_lowest_swr(init))
			tuning_new_state(TUNING_DONE);
		break;
	case TUNING_DONE:
		if (!best_swr_found) {
			printinfo("SWR nicht gefunden\n");
			tuning_new_state(NO_TUNING);
			break;
		}
		saveTuningValue();
		// das gewünschte swr wurde gefunden, beende
		best_swr_found = 0;
		tuning_new_state(NO_TUNING);
		printinfo("Scan FERTIG\n");
		uart_civ_tuning_done(0);
		insert_prot();
		lastfswr = bestswr;
		break;
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
	tuning_new_state(nr);
}

// steuert das Tuning eines ganzen Bandes
void tune_fullband()
{
	static uint32_t startqrg, stopqrg, actqrg;
	static uint32_t wait_qrg = 0;
	uint32_t civ_freq = getCIVfreq();

	if(!tuning_band)
		return;
	if(wait_qrg) {
		// eine freq wurde im TRX eingestellt, warte bis diese bestätigt wird
		// dann starte das Scannen
		if(wait_qrg == civ_freq) {
			tuning_single = 1;
			wait_qrg = 0;
		} else {
			// die QRG stimmt noch nicht
			// warte einen Moment und fordere sie nochmal an
			delay_1ms(50);
			cat_setqrg_Hz(actqrg);
		}
	}

	if(old_tuning_band == 0) {
		// das Band Tuning wurde soeben eingeschaltet
		// Initialisiere
		lastfswr = 9999;
		old_tuning_band = 1;
		if(fswr == 0) {
			printinfo("Bitte TX einschalten\n");
			stop_tuning();
			return;
		}
		printinfo("Starte Band-TUNING\n");
		no_tuneFromMem = 1;
		getFreqEdges(civ_freq , &startqrg, &stopqrg);
		// stelle die erste qrg ein
		actqrg = startqrg;
		cat_setqrg_Hz(actqrg);
		wait_qrg = actqrg;
	}

	if(tuning_single == 0 && wait_qrg == 0)	{
		// die Frequenz ist fertig, gehe zur nächsten
		actqrg += 10000;
		if(actqrg > stopqrg) {
			// fertig
			cat_txrx(0);
			printinfo("Bandscan FERTIG\n");
			tuning_band = 0;
			no_tuneFromMem = 0;
			old_tuning_band = 0;
			return;
		}
		cat_setqrg_Hz(actqrg);
		wait_qrg = actqrg;
	}
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

// scanne die aktuell eingestellte QRG
void seek_single()
{
	uint32_t civ_freq = getCIVfreq();
	if(civ_freq == 0)
		return;

	cat_txrx(1);
//	delay_1ms(500);

	tuning_single = 1;
}

// Tune das komplette aktuelle Band in 10kHz Schritten
void seek_band()
{
	uint32_t civ_freq = getCIVfreq();

	if(civ_freq == 0)
		return;

	cat_txrx(1);
	delay_1ms(500);

	tuning_band = 1;
}

void stop_tuning()
{
	tuning_new_state(NO_TUNING);
	lastproc = NO_TUNING;
	cat_txrx(0);
	printinfo("Band-Scan STOP\n");
}

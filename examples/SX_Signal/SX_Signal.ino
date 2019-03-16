/*
 Signal-Dekoder für US-Signale
 "SX-Signal"

 HW         = SX-Signal (1.0)
 SW Version:    0.2

 *  Created on: 16 Mar 2019
 *  Changed on: see git
 *  Copyright:  Michael Blank

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 */

#define TESTVERSION  // aktiviert serielle Testausgaben und Dialog über Serial Interface

#include <SoftPWM.h>
#include <SX22b.h>   // this is the Selectrix library
#include <EEPROM.h>
#include <OneButton.h>

// für den Programmiervorgang über den SX-Bus:
#define  DEFAULTADDR    71   // default address for 4 chan US-signal decoder
#define  PROGLED      13  
#define  PROGBUTTON   3     // for entering programming mode
#define  DEBOUNCETIME 200
#define  EEPROMSIZE 1                  // 1 Byte vorsehen: Adresse

char swversion[] = "SW V00.2";
char hwversion[] = "HW V01.1";
#define N_ASP  3 
#define N_SIG  4
uint8_t sig[N_SIG][N_ASP] = { { A5, A3, A4 }, { A1, A0, A2 }, { 11, 10, 12 }, 
                  { 8, 9, 7 } }; // Sig0 ... Sig3
uint8_t st[N_SIG], last[N_SIG];

int DecoderSXAddr;
int err;

byte SXData;
byte SXOut;

SX22b sx;                // library
//SX22Command sxcmd;       // holds command data

static byte oldSXData;

// für den Programmiervorgang über den SX-Bus:
boolean programming = false;
OneButton progBtn(PROGBUTTON, true);  // 0 = key pressed

#define  MAXPROGPARAM 1

int SXtmp[MAXPROGPARAM], oldSXtmp[MAXPROGPARAM], checkedSXtmp[MAXPROGPARAM],
		orgSXVal[MAXPROGPARAM];

int progValLimits[MAXPROGPARAM][2] = { { 1, 102 } // SXAddress    lower/upper limit
};

byte trackOn;

volatile boolean running = false;

void sxisr(void) {
	// if you want to understand this, see:
	// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239522239

	sx.isr();
	running = true;
}

void setup() {

#ifdef TESTVERSION
	Serial.begin(115200);
	delay(10);
	Serial.println(swversion);
	Serial.println(hwversion);
#endif

	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleProgramming);
	progBtn.setPressTicks(1000);  // button long press after 1 second

	running = false;
	SXOut = 0;

	//**************** init pins and SoftPWM lib
	SoftPWMBegin();
	for (int i = 0; i < N_ASP; i++) {
		for (uint8_t j = 0; j < N_SIG; j++) {
			SoftPWMSet(sig[j][i], 255);   // off
		}
	}

	SoftPWMSetFadeTime(ALL, 800, 800);

	// switch red(hp0) on for all signals
	for (uint8_t j = 0; j < N_SIG; j++) {
		last[j] = 0;
		SoftPWMSet(sig[j][0], 0);
	}

	//**** init EEPROM and Decoder Address
	if (!EEPROMEmpty()) {
		EEPROMRead();
	} else {
		DecoderSXAddr = DEFAULTADDR;
		EEPROMSave();
	}

	//**** init interrupt
	sx.init();
	attachInterrupt(0, sxisr, RISING);            // RISING edge
}

//******************************************************************************************
boolean EEPROMEmpty() {

	int val;
	boolean empty = true;
	int i = 0;

	while (empty && (i < EEPROMSIZE)) {
		val = EEPROM.read(i);
		empty = val == 255;
		i++;
	}

#ifdef TESTVERSION
	Serial.print(F("EEPROMEmpty: "));
	if (empty)
		Serial.println(F("yes"));
	else
		Serial.println(F("no"));
#endif

	return empty;
}

//******************************************************************************************
void EEPROMSave() {

	EEPROM.write(0, DecoderSXAddr);

}

//******************************************************************************************
void EEPROMRead() {

	DecoderSXAddr = EEPROM.read(0);

#ifdef TESTVERSION
	Serial.println(F("EEPROMRead:"));
	Serial.print(F("DecoderSXAddr="));
	Serial.println(DecoderSXAddr);
	Serial.println();
#endif
}

//******************************************************************************************
void saveOldSXValues() {

	for (int i = 0; i < MAXPROGPARAM; i++) {
		orgSXVal[i] = sx.get(i + 1);
#ifdef TESTVERSION
		Serial.print(orgSXVal[i]);
		Serial.print(" ");
#endif 
	}
#ifdef TESTVERSION
	Serial.println();
#endif 
}

//******************************************************************************************
void restoreOldSXValues() {

	for (int i = 0; i < MAXPROGPARAM; i++) {
		while (sx.set(i + 1, orgSXVal[i]))
			;
		delay(50);
#ifdef TESTVERSION
		Serial.print(orgSXVal[i]);
		Serial.print(" ");
#endif 
	}
#ifdef TESTVERSION
	Serial.println();
#endif 

}

//******************************************************************************************
void toggleProgramming() {
	// called when button is long pressed
#ifdef TESTVERSION
	Serial.print(F("toggleProgramming, trackOn="));
	Serial.println(trackOn);
#endif

	if (!trackOn) {
		// react to key press only when track voltage is off
		if (!programming) {
			startProgramming();
		} else {
			finishProgramming();
		}
	} // !trackOn
}

//******************************************************************************************
void startProgramming() {

#ifdef TESTVERSION
	Serial.println(F("startProgramming"));
#endif 
	digitalWrite(PROGLED, HIGH);
	programming = true;
	saveOldSXValues();

	// MAXPROGRPARAM = 1
	while (sx.set(0 + 1, DecoderSXAddr))
		;
	SXtmp[0] = DecoderSXAddr;
	oldSXtmp[0] = DecoderSXAddr;
	checkedSXtmp[0] = DecoderSXAddr;

}

//******************************************************************************************
void finishProgramming() {

#ifdef TESTVERSION
	Serial.println(F("finishProgramming"));
#endif
	DecoderSXAddr = checkedSXtmp[0];

	EEPROMSave();
	EEPROMRead();
	restoreOldSXValues();
	delay(80);
	programming = false;
	digitalWrite(PROGLED, LOW);
}

//******************************************************************************************
void processProgramming() {

	for (int i = 0; i < MAXPROGPARAM; i++) {
		SXtmp[i] = sx.get(i + 1);
		if (SXtmp[i] != oldSXtmp[i]) {        // value has been changed
			if ((SXtmp[i] >= progValLimits[i][0])
					&& (SXtmp[i] <= progValLimits[i][1])) { // within predefined limits?
				checkedSXtmp[i] = SXtmp[i];       // accept value
				oldSXtmp[i] = SXtmp[i];
			} else {  // out of limits
				SXtmp[i] = oldSXtmp[i];           // reset value
				while (sx.set(i + 1, SXtmp[i]))
					;     // also on the SX-Bus
			}
		}
	}
}

//******************************************************************************************
void setSignalState(uint8_t n, uint8_t st) {
	if (n >= N_SIG)
		return;  // error

	if ((st >= 0) && (st <= 3)) {
		// switch last off
		SoftPWMSet(sig[n][last[n]], 255);
		// swith new state on
		if (st != 3) {
			if ((last[n] != 0) && (st != 0)) {
				// first switch to red
				SoftPWMSet(sig[n][0], 0);  // red on
				delay(1000);
				SoftPWMSet(sig[n][0], 255);  // red off
			}
			SoftPWMSet(sig[n][st], 0);
		}
		last[n] = st;
	}
}

//******************************************************************************************
void loop() {

	progBtn.tick();

	if (running) {
		running = false;

		trackOn = sx.getTrackBit();
		if (trackOn) {                        // Tracksignal = On
			if (programming) {
				finishProgramming();
			} else {

				// check selectrix channels for changes
				byte d = sx.get(DecoderSXAddr);
				byte val = 0;

				if (d != oldSXData) {
					for (uint8_t sig = 0; sig < N_SIG; sig++) {
						switch (sig) {
						case 0:
							val = (d & 0x03);  // last 2 sx bits (1,2)
							if (val != last[sig])
								setSignalState(0, val);
							break;
						case 1:
							val = ((d >> 2) & 0x03);  // sx bit 3,4
							if (val != last[sig])
								setSignalState(0, val);
							break;
						case 2:
							val = ((d >> 4) & 0x03);  // sx bit 5,6
							if (val != last[sig])
								setSignalState(0, val);
							break;
						case 3:
							val = ((d >> 6) & 0x03);  // sx bit 7,8
							if (val != last[sig])
								setSignalState(0, val);
							break;
						}
					}
#ifdef TESTVERSION
					Serial.print("d=");
					Serial.println(d,HEX);
#endif
					oldSXData = d;
				}
			}
		} else {                              // Tracksignal = Off
			if (programming) { // process the received data from SX-bus
				processProgramming();
			}
		}    // TrackSignal = Off
	}   // running
}

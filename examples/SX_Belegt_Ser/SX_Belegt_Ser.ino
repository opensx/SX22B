/*******************************************************************************************
 * SX_Belegt_Ser
 *
 * Created:    09.10.2018
 * Copyright:  Michael Blank / Reinhard Thamm
 * Version:    V01.00
 *
 * Libraries: SX22: Quellen von M.Blank mit Ideen von U.Beyenbach und Änderungen von R.Thamm
 * Zielhardware/-controller: Arduino Pro Mini (5V, 16MHz), ATmega328
 * Aufsatz:  SX_Belegt_210 (serieller Ausgang 9600 Baud, m. ATtiny84, Beisp.: "S0456000")
 *
 * Pin2=T0=INT0, Pin4=T1, Pin6=SX-Write
 *
 * Zum Programmieren des Moduls am SX-Bus werden die SX-Kanäle 1 bis 5 verwendet:
 *
 *   SX-Kanal 1 : SX-Adresse dieses Moduls (0..102)
 *   SX-Kanal 2 : Settings1 (hier: Abfallverzögerungszeit (0..255), mit 25ms/digit, also
 *                                                         20 entspricht 500ms
 *   SX-Kanal 3 : Settings2 (hier: Empfindlichkeit 1 ... 9, 9=unempfindlich, 4=default)
 *   SX-Kanal 4 : HW/SW-Version (hier nicht benutzt)
 *   SX-Kanal 5 : Modultyp (hier nicht benutzt)
 *
 * SX-Adresse des Moduls, Abfallverzögerung und Empfindlichkeit werden im EEPROM des
 * Controllers abgespeichert.
 *

 * Changes:
 *
 *******************************************************************************************/
#define TESTVERSION  // aktiviert serielle Testausgaben und Dialog über Serial Interface

#include "Arduino.h"
#include <SX22b.h>  // Pin2=T0=INT0, Pin4=T1, Pin6=SX-Write
#include <EEPROM.h>
#include <OneButton.h>

SX22b sx;

#define  DEFAULTADDR  98               // die Adresse, auf die dieses Modul hört
#define  DEFAULTCODEDDELAYTIME 20      // *25ms = 500ms Abfallverzögerung
#define  DELAYTIMEFACTOR 25            // 1 digit entspricht 25ms
#define  DEFAULT_SENS  4

#define  TESTTAKT 1000
#define  LEDTAKT 250
#define  PROGLED 13
#define  PROGBUTTON 3
#define  DEBOUNCETIME 200

#define  EEPROMSIZE 3                  // 3 Byte vorsehen: Adresse, Abfallverzögerung, Empf.

char swversion[] = "SW V01.00";
char hwversion[] = "HW V01.02";    // basisboard RJ45-1.1 oder RJ45-1.2

int SXAddr;
int err;

byte SXData;
byte SXOut;

byte DataPin[8] = { 5, 7, 8, 9, 10, 11, 12, A0 }; // Pindefinitionen  TODO

byte DataRead[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // gelesene Daten   TODO
byte DataOut[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // zur Meldung a.d. Zentrale
unsigned long lastOccupied[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // Zeitpunkt letzter Belegung

unsigned delayTime;               // Verzögerungszeit gegen Melderflackern
int codedDelayTime;       // Abbildung in einem Byte: pro Digit=25ms, hier 500ms
//   dieser Wert wird im EEPROM abgelegt
byte sensity;                 // Empfindlichkeit

// für den Programmiervorgang über den SX-Bus:
boolean programming = false;
OneButton progBtn(PROGBUTTON, true);  // 0 = key pressed

#define  MAXPROGPARAM 5
const byte SXAddress = 0;
const byte SXSettings1 = 1;
const byte SXSettings2 = 2;
const byte SXHWSWVersion = 3;
const byte SXModuleType = 4;

int SXtmp[MAXPROGPARAM], oldSXtmp[MAXPROGPARAM], checkedSXtmp[MAXPROGPARAM],
		orgSXVal[MAXPROGPARAM];

int progValLimits[MAXPROGPARAM][2] = { { 1, 102 }, // SXAddress    lower/upper limit
		{ 2, 255 },  // SXSettings1  min 50msec
		{ 1, 9 },    // SXSettings2
		{ 0, 0 },    // SXHWSWVersion
		{ 0, 0 } };   // SXModuleType

unsigned long time1;

byte trackOn;

volatile boolean running = false;

byte inputBuf[20];

//******************************************************************************************
void sxisr(void) {
	// if you want to understand this, see:
	// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239522239

	sx.isr();
	running = true;
}

//******************************************************************************************
void setup() {
	Serial.begin(9600);
	Serial.setTimeout(200);

#ifdef TESTVERSION
	time1 = millis();
	delay(10);
	Serial.println(swversion);
	Serial.println(hwversion);
#endif

	for (int i = 0; i < 8; i++) {
		pinMode(DataPin[i], INPUT);
		digitalWrite(DataPin[i], HIGH);
		lastOccupied[i] = millis();
	}

	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleProgramming);
	progBtn.setPressTicks(1000);  // button long press after 1 second

	running = false;
	SXOut = 0;

	if (!EEPROMEmpty()) {
		EEPROMRead();
	} else {
		SXAddr = DEFAULTADDR;
		codedDelayTime = DEFAULTCODEDDELAYTIME;
		delayTime = codedDelayTime * DELAYTIMEFACTOR;
		sensity = DEFAULT_SENS;
		EEPROMSave();
	}

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

	EEPROM.write(0, SXAddr);
	EEPROM.write(1, codedDelayTime);
	EEPROM.write(2, sensity);
}

//******************************************************************************************
void EEPROMRead() {

	SXAddr = EEPROM.read(0);
	codedDelayTime = EEPROM.read(1);
	delayTime = codedDelayTime * DELAYTIMEFACTOR;
	sensity = EEPROM.read(2);
	if ((sensity < 1) || (sensity > 9)) {
		sensity = DEFAULT_SENS;
	}

#ifdef TESTVERSION
	Serial.println(F("EEPROMRead:"));
	Serial.print(F("SXAddr="));
	Serial.println(SXAddr);
	Serial.print(F("CodedDelayTime="));
	Serial.println(codedDelayTime);
	Serial.print(F("delayTime="));
	Serial.println(delayTime);
	Serial.print(F("sensity="));
	Serial.println(sensity);
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

	for (int i = 0; i < MAXPROGPARAM; i++) {
		switch (i) {
		case SXAddress:
			while (sx.set(i + 1, SXAddr))
				;
			SXtmp[i] = SXAddr;
			oldSXtmp[i] = SXAddr;
			checkedSXtmp[i] = SXAddr;
			break;
		case SXSettings1:
			while (sx.set(i + 1, codedDelayTime))
				;
			SXtmp[i] = codedDelayTime;
			oldSXtmp[i] = codedDelayTime;
			checkedSXtmp[i] = codedDelayTime;
			break;
		case SXSettings2:
			while (sx.set(i + 1, sensity))
				;
			SXtmp[i] = sensity;
			oldSXtmp[i] = sensity;
			checkedSXtmp[i] = sensity;
			break;
		case SXHWSWVersion:
			break;
		case SXModuleType:
			break;
		}  // switch (i)
	}
}

//******************************************************************************************
void finishProgramming() {

#ifdef TESTVERSION
	Serial.println(F("finishProgramming"));
#endif
	SXAddr = checkedSXtmp[0];
	codedDelayTime = checkedSXtmp[1];
	sensity = checkedSXtmp[2];
	EEPROMSave();
	EEPROMRead();
	restoreOldSXValues();
	delay(80);
	programming = false;
	digitalWrite(PROGLED, LOW);
}

//******************************************************************************************
// read serial port until NEWLINE is received with a timeout of 200msec
void checkSerial() {
	int n = Serial.readBytesUntil('\n', inputBuf, 19);
	if ((inputBuf[0] == 'S') && (n >= 10)) {
		// update only when we have a complete result for 8 channels
		// with correct starting char 'S'
		//   like "S02300560"
		Serial.print("rec: ");
		Serial.print(n);
		Serial.print(' ');
		for (int j = 0; j < n; j++) {
			Serial.write(inputBuf[j]);
		}
		Serial.println();

		for (int i = 0; i < 8; i++) {
			if ((inputBuf[i+1] - '0') >=  sensity) {  // skip 'S' character
				DataRead[i] = 0;  // => belegt
			} else {
				DataRead[i] = 1;
			}

			if (!DataRead[i]) { // --> belegt!

				DataOut[i] = DataRead[i];
				lastOccupied[i] = millis();
			} else {            // frei!
				if (millis() > lastOccupied[i] + delayTime) { // delayTime abgelaufen?
					DataOut[i] = DataRead[i];
				}
			}
			bitWrite(SXOut, i, !DataOut[i]);
		}

	}
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
void loop() {

	int x;

	progBtn.tick();

	if (running) {
		running = false;
		checkSerial();
		trackOn = sx.getTrackBit();
		if (trackOn) {                        // Tracksignal = On
			if (programming) {
				finishProgramming();
			} else {    // not programming
				SXData = sx.get(SXAddr);
				if (SXData != SXOut) {
					x = sx.set(SXAddr, SXOut);
				}
			}  // not programming
		} else {                              // Tracksignal = Off

			if (programming) {          // process the received data from SX-bus
				processProgramming();
			} else {             // not programming
				SXData = sx.get(SXAddr);
				if (SXData != 0) {
					x = sx.set(SXAddr, 0);    // NO occupation signal in case of
				}                           //    TrackSignal = Off
			}

		}    // TrackSignal = Off
	}   // running

#ifdef TESTVERSION
	if (millis() > time1 + TESTTAKT) {
		time1 = millis();

		for (int i = 7; i >= 0; i--) {
			Serial.print(bitRead(SXOut, i));
		}
		Serial.print(F("  "));
		Serial.println(SXOut);
	}

	while (Serial.available()) {
		char inChar = (char) Serial.read();

		switch (inChar) {
		case 'd':                // dump data
			break;
		} // switch
	}  // while Serial
#endif
}            //loop

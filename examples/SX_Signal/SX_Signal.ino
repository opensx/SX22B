

/*
 Signal-Dekoder für US-Signale
 "SX-Signal"

 HW         = SX-Signal (1.0)
 SW Version:    0.4

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
#define PWM_ON

#ifdef PWM_ON
#include "SoftPWM_SX12.h"

#endif

#include <SX22b.h>   // this is the Selectrix library
#include <EEPROM.h>
#include <OneButton.h>

// für den Programmiervorgang über den SX-Bus:
#define  DEFAULTADDR    71   // default address for 4 chan US-signal decoder
#define  PROGLED      13  
#define  PROGBUTTON   3     // for entering programming mode
#define  DEBOUNCETIME 200
#define  EEPROMSIZE 1                  // 1 Byte vorsehen: Adresse

char swversion[] = "SW V00.4";
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

static uint32_t time0 = 0;

void sxisr(void) {
	// if you want to understand this, see:
	// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239522239

	sx.isr();
	running = true;
#ifdef PWM_ON
  SoftPWM_Timer_Interrupt();
#endif
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

#ifdef PWM_ON
  //**************** init pins and SoftPWM lib
  SoftPWMBegin(SOFTPWM_INVERTED);
  for (int i = 0; i < N_ASP; i++) {
    for (uint8_t j = 0; j < N_SIG; j++) {
      SoftPWMSet(sig[j][i], 0);   // off
    }
  } 

  SoftPWMSetFadeTime(ALL, 400, 400);     // 200,800

  // switch red(hp0) on for all signals
  for (uint8_t j = 0; j < N_SIG; j++) {
    last[j] = 0;
    SoftPWMSet(sig[j][0], 255);
  }   

  

#else
  for (uint8_t j=0 ; j < N_SIG; j++) {
    for (uint8_t i= 0 ; i < N_ASP; i++) {
       pinMode(sig[j][i], OUTPUT);
       if (i == 0) {
         digitalWrite(sig[j][i], LOW);   // turn the RED LED on 
       } else {
         digitalWrite(sig[j][i], HIGH);   // turn other LEDs off  
       }                 
    }
    last[j] = 0;
  }
#endif  
  
  
	
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
#ifdef PWM_ON
void setSignalState(uint8_t nsig, uint8_t state) {
  if (nsig >= N_SIG)
    return;  // error
  if (state == last[nsig])
    return;  // do nothing, no change
    
  switch (state) {
    case 0:
      if ((last[nsig] == 1) || (last[nsig] == 2) ) {       
        SoftPWMSet(sig[nsig][last[nsig]], 0);
      }
      SoftPWMSet(sig[nsig][state], 255);  // red on
      break;
    case 1:
      if (last[nsig] ==  2) {       
        SoftPWMSet(sig[nsig][last[nsig]], 0);
        delay(200);
        // first switch to red
        SoftPWMSet(sig[nsig][0], 255);  // red on
        delay(1000);
      }
      SoftPWMSet(sig[nsig][0], 0);  // red off  
      SoftPWMSet(sig[nsig][state], 255);   // green on
      break;
    case 2:
      if (last[nsig] ==  1) {       
        SoftPWMSet(sig[nsig][last[nsig]], 0);
        delay(200);
        // first switch to red
        SoftPWMSet(sig[nsig][0], 255);  // red on
        delay(1000);
      }
      SoftPWMSet(sig[nsig][0], 0);  // red off
      SoftPWMSet(sig[nsig][state], 255);   // yellow on
      break;
    case 3:
      SoftPWMSet(sig[nsig][0], 0);  // red off
      SoftPWMSet(sig[nsig][1], 0);  // green off
      SoftPWMSet(sig[nsig][2], 0);  // yellow off
      break;
    
  }
}  
#else
void setSignalState(uint8_t nsig, uint8_t state) {
  if ((nsig >= N_SIG) || (state > N_ASP)) return;
  for (uint8_t i= 0 ; i < N_ASP; i++) {
    pinMode(sig[nsig][i], OUTPUT);
    digitalWrite(sig[nsig][i], HIGH);   // turn the LEDs off                   
  }
  if (state < N_ASP) {
    digitalWrite(sig[nsig][state], LOW);
  }  // else: all LEDs off

}
#endif

//******************************************************************************************

void processSignals() {
/* TEST static uint8_t redon = 0;
     if ((millis() - time0) > 2000) {
        //SoftPWMSet(sig[0][0], 0);
        time0 = millis();
        if (redon) {
           SoftPWMSet(sig[0][0], 0);
           redon = 0;
        } else {
           SoftPWMSet(sig[0][0], 255);
           redon = 1;
        }
        Serial.println(redon);
     }
     return;  */
  
    // check selectrix channels for changes
        byte d = sx.get(DecoderSXAddr);
        byte val = 0;

        if (d != oldSXData) {
          for (uint8_t sig = 0; sig < N_SIG; sig++) {
            switch (sig) {
            case 0:
              val = (d & 0x03);  // last 2 sx bits (1,2)
              if (val != last[0]) {
                 setSignalState(0, val);
                 last[0] = val;
              }
              break;
            case 1:
              val = ((d >> 2) & 0x03);  // sx bit 3,4
              if (val != last[1]) {
                 setSignalState(1, val);
                 last[1] = val;
              }
              break;
            case 2:
              val = ((d >> 4) & 0x03);  // sx bit 5,6
              if (val != last[2]) {
                 setSignalState(2, val);
                 last[2] = val;
              }
              break;
            case 3:
              val = ((d >> 6) & 0x03);  // sx bit 7,8
              if (val != last[3]) {
                 setSignalState(3, val);
                 last[3] = val;
              }
              break;
            }
          } 

 
#ifdef TESTVERSION
          Serial.print("d=0x");
          Serial.println(d,HEX);
#endif
          oldSXData = d;
        }

}

//******************************************************************************************
void loop() {

	progBtn.tick();

	if (running) {
		running = false;

    processSignals();
    
		trackOn = sx.getTrackBit();
		if (trackOn) {                        // Tracksignal = On
			if (programming) {
				finishProgramming();
			} 
		} else {                              // Tracksignal = Off
			if (programming) { // process the received data from SX-bus
				processProgramming();
			}
		}    // TrackSignal = Off
	}   // running
}

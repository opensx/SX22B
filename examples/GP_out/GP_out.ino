/*
 * GP_out (General Purpose Outputs)
 * 
 8x 5V Ausgänge f. Basisplatine
 
 HW         = Basisboard-RJ45-1.1
 HW aufsatz = RT_Weiche_Signal 0.1 mit 0 Ohm
 SW Version:    0.1
 
 *  Created on: 09 Dec 2016
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
#include <Arduino.h> 
#include <EEPROM.h>

#include "hwConfig.h"
#include "sxpin_def.h"


#include <SX22b.h>   // this is the Selectrix library


// für den Programmiervorgang über den SX-Bus:
#define PROGLED      13  
#define PROGBUTTON   3     // for entering programming mode
#define DEBOUNCETIME 200
boolean programming = false;

//======= take configuration from hwConfig. h ================================

//================== config end ===============================================

//uint32_t timer = 0;
//uint32_t greenTime = 0;
uint32_t keyPressTime = 0;

SX22b sx;                // library
//SX22Command sxcmd;       // holds command data

static byte oldSXData, SXAddr;



void sxisr(void) {
	// if you want to understand this, see:
	// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239522239   
	sx.isr();
	
}

void setup() {
	programming = false;
	pinMode(PROGLED, OUTPUT);
	digitalWrite(PROGLED, LOW);
	pinMode(PROGBUTTON, INPUT);
	digitalWrite(PROGBUTTON, HIGH);  // pull up

  pinMode(SX_BIT8, OUTPUT);
  pinMode(SX_BIT7, OUTPUT);
  pinMode(SX_BIT6, OUTPUT);
  pinMode(SX_BIT5, OUTPUT);
  pinMode(SX_BIT4, OUTPUT);
  pinMode(SX_BIT3, OUTPUT);
  pinMode(SX_BIT2, OUTPUT);
  pinMode(SX_BIT1, OUTPUT);
  
	// check whether there is a valid decoder address in EEPROM
	SXAddr = EEPROM.read(EEPROM_ADDR);
	if ((SXAddr == 0) || (SXAddr > MAX_ADDR)) {    // never stored before or invalid
		SXAddr = DECODER_ADDR;   // use default address
		EEPROM.write(EEPROM_ADDR, SXAddr);  // store address
		delay(10);
	}

	sx.init();

	// RISING slope on INT0 triggers the interrupt routine sxisr (see above)
	attachInterrupt(0, sxisr, RISING);
}

void startModuleProgramming() {
	programming = true;
	keyPressTime = millis();
	digitalWrite(PROGLED, HIGH);
	
	// try to write current address on the sx-bus
	// returns "0" when successful
	while (sx.set(SX_CHAN_ADDR, SXAddr) != 0) { 
		delay(10);
	}

}

void finishModuleProgramming() {
	programming = false;
	uint8_t newAddress = sx.get(SX_CHAN_ADDR);   //get sx data from bus
	if ((newAddress > 0) && (newAddress <= MAX_ADDR)) {
		SXAddr = newAddress;   // use default address
		EEPROM.write(EEPROM_ADDR, SXAddr);  // store address
		delay(10);	
	}
	digitalWrite(PROGLED, LOW);
}

boolean keypressed() {
	if ((millis()- keyPressTime) < 5*DEBOUNCETIME) {
		// totzeit von 1sec
		return false;
	}
	if (!digitalRead(PROGBUTTON)) {
		delay(DEBOUNCETIME);
		if (!digitalRead(PROGBUTTON)) {
			keyPressTime = millis();
			return true;
		}
	} else {
		return false;
	}
}

void loop() {
	
	// check selectrix channels for changes
	byte d = sx.get(SXAddr);

	if (d != oldSXData) {
    // SX bits are usually numbered from 1...8
		SX_BIT8_(bitRead(d,7));
    SX_BIT7_(bitRead(d,6));
    SX_BIT6_(bitRead(d,5));
    SX_BIT5_(bitRead(d,4));
    SX_BIT4_(bitRead(d,3));
    SX_BIT3_(bitRead(d,2));
    SX_BIT2_(bitRead(d,1));
    SX_BIT1_(bitRead(d,0));
		
		oldSXData = d;
	}

	uint8_t track = sx.getTrackBit();
	if (programming) { 
		if (track || keypressed() ) {
			// finish programming when track voltage is enabled again
			finishModuleProgramming();
		} 
	} else if ( (track == 0) && keypressed()) {
		// start programming only if track voltage is disabled and
		// key is pressed and not already programming
		startModuleProgramming();
	}
	
}


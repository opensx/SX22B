/*
 * GP_out (General Purpose Outputs)
 * 
 *  8x 5V Ausgänge f. Basisplatine
 *  mit Leuchtstoffröhren flackern beim Einschalten
 
 HW         = Basisboard-RJ45-1.1
 HW aufsatz = RT_Weiche_Signal 0.1 mit 0 Ohm
 SW Version = 0.2
 
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
#include <SX22b.h>   // this is the Selectrix library

#define HARDWARE   "HW: GP_OUT_8"    // 2 signals and 4 turnouts
#define SOFTWARE   "SW: GP_out 0.2"
//  Platine E863820, Common Ground, direct output from ATmega328 used
//  R = 0 Ohm

#define DECODER_ADDR  (88)   // 
#define N_CHAN        (8)    // 8 output channels
#define MAX_ADDR     (108)   // maximum valid decoder address

// how to interpret sx programming values
#define SX_CHAN_ADDR     (1)   // SX address channel
#define SX_CHAN_FLICKER  (2)   // SX channel for flicker on/off

#define EEPROM_ADDR    (1)   // eeprom address for storing decoder address
#define EEPROM_FLICKER (2)   // eeprom address for storing decoder address
#define FLICKER_INIT   (0)   // no flicker

// für den Programmiervorgang über den SX-Bus:
#define PROGLED      13  
#define PROGBUTTON   3     // for entering programming mode
#define DEBOUNCETIME 200
boolean programming = false;

//#define _DEBUG    // if defined => serial output for debugging

//======= configuration of pins, see rt_weiche_signal =========================

// 
uint8_t pinFromSX[N_CHAN] = { A0, 12, 11, 10, 9, 8, 7, 5 };

uint8_t flicker = FLICKER_INIT;  // flourescent flickering

long flickerTimer[N_CHAN];

//================== config end ===============================================

uint32_t keyPressTime = 0;

SX22b sx;                // library

static byte SXAddr;

void sxisr(void) {
	// if you want to understand this, see:
	// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239522239   
	sx.isr();
}

void setup() {
#ifdef _DEBUG
	Serial.begin(9600);
	Serial.println(HARDWARE);
	Serial.println(SOFTWARE);
#endif
	programming = false;
	pinMode(PROGLED, OUTPUT);
	digitalWrite(PROGLED, LOW);
	pinMode(PROGBUTTON, INPUT);
	digitalWrite(PROGBUTTON, HIGH);  // pull up

	for (uint8_t i = 0; i < N_CHAN; i++) {
		pinMode(pinFromSX[i], OUTPUT);
		digitalWrite(pinFromSX[i], LOW);  // =OFF
		flickerTimer[i] = 0;
	}

	// check whether there is a valid decoder address in EEPROM
	SXAddr = EEPROM.read(EEPROM_ADDR);
	flicker = EEPROM.read(EEPROM_FLICKER);

#ifdef _DEBUG
	Serial.print("adr =");
	Serial.print(SXAddr);
	Serial.print(" flicker=");
	Serial.println(flicker);
#endif
	
	if ((SXAddr == 0) || (SXAddr > MAX_ADDR)) { // never stored before or invalid
		SXAddr = DECODER_ADDR;   // use default address
		EEPROM.write(EEPROM_ADDR, SXAddr);  // store address
		delay(20);
		flicker = FLICKER_INIT;
		EEPROM.write(EEPROM_FLICKER, flicker);  // init flicker
		delay(20);
	}
	
#ifdef _DEBUG
	Serial.print("adr =");
	Serial.print(SXAddr);
	Serial.print(" flicker=");
	Serial.println(flicker);
#endif
	
	randomSeed (analogRead(A2));    // randomize

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

	// try to write current "flicker" on the sx-bus
	// returns "0" when successful
	while (sx.set(SX_CHAN_FLICKER, flicker) != 0) {
		delay(10);
	}

}

void finishModuleProgramming() {
	programming = false;
	uint8_t newAddress = sx.get(SX_CHAN_ADDR);   //get sx data from bus
	uint8_t flicker = sx.get(SX_CHAN_FLICKER);

	if ((newAddress > 0) && (newAddress <= MAX_ADDR)) {
		SXAddr = newAddress;   // use default address
		EEPROM.write(EEPROM_ADDR, SXAddr);  // store address
		delay(20);

		EEPROM.write(EEPROM_FLICKER, flicker);  // store flicker bits
		delay(20);
	}
	digitalWrite(PROGLED, LOW);
}

boolean keypressed() {
	if ((millis() - keyPressTime) < 5 * DEBOUNCETIME) {
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

	// check selectrix channel
	byte d = sx.get(SXAddr);
#ifdef _DEBUG
	static byte oldD = 0;
	if (oldD != d) {
	  Serial.print("d=");
	  Serial.println(d);
	  oldD = d;
	}
#endif
	// SX bits are usually numbered from 1...8, but we are using 0..7 here
	for (uint8_t i = 0; i < N_CHAN; i++) {

		// if corresponding bit in sx-value is set
		if (bitRead(d, i)) {
			if (bitRead(flicker, i)   // check if flicker bit is set
			    && ((millis() - flickerTimer[i]) < 3000)  // only for 3 secs
				) {
				// do flicker on
				int del = random(30, 100);
				delay(del);
				int onoff = random(0, 100);
				if (onoff > 80) {
					digitalWrite(pinFromSX[i], HIGH);
				} else {
					digitalWrite(pinFromSX[i], LOW);
				}
			} else {
				// no flicker, just on
				digitalWrite(pinFromSX[i], HIGH);
			}

		} else {
			// bit is off
			digitalWrite(pinFromSX[i], LOW);
			// reset flicker timer
			flickerTimer[i] = millis();
		}

	}

	uint8_t track = sx.getTrackBit();
	if (programming) {
		if (track || keypressed()) {
			// finish programming when track voltage is enabled again
			finishModuleProgramming();
		}
	} else if ((track == 0) && keypressed()) {
		// start programming only if track voltage is disabled and
		// key is pressed and not already programming
		startModuleProgramming();
	}

}


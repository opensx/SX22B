/*
 Weichen- und Signal-Dekoder Aufsatz-Dekoder f. Basisplatine

 
 *  Created on: 30 Nov 2016
 *  Changed on: 
 *  Version:    0.1
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
//#include "Signal.h"
#include "Turnouts.h"
#include "hwConfig.h"
#include "signal_def.h"

#include <SX22b.h>   // this is the Selectrix library
#include <SX22Command.h>   // this is the Selectrix Command library

#define LED_PIN  13   // on most Arduinos there is an LED at pin 13



//======= take configuration from hwConfig. h ================================


//================== config end ===============================================

long timer = 0;
long greenTime = 0;

SX22b sx;                // library
//SX22Command sxcmd;       // holds command data

static int ledState = LOW;
static byte oldSX[2];
volatile byte val = 0, val1 = 0;

// A0 beim Arduino

void sxisr(void) {
    // if you want to understand this, see:
    // http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239522239   
    sx.isr();
static uint8_t fast = 0;
    fast++;

       // counting with higher 4 bits of "fast"
      if (val > (fast >> 4)) {
        SIG_A_R_(HIGH);
      } else {
        SIG_A_R_(LOW);
      }

      if (val1 > (fast >> 4)) {
        SIG_A_G_(HIGH);
      } else {
        SIG_A_G_(LOW);
      }
    
} 

void setup() {
    
    pinMode(LED_PIN,OUTPUT);
    pinMode(SIG_A_R,OUTPUT);
    pinMode(SIG_A_G,OUTPUT);
    // init signal and turnout outputs
    //sigs[0].init();
    //sigs[1].init();
    turnouts.init(enablePin);
    
    sx.init();

    // RISING slope on INT0 triggers the interrupt routine sxisr (see above)
    attachInterrupt(0, sxisr, RISING); 
} 


void loop() {
  //sigs[0].process();   // for led fading
  //sigs[1].process();

  
  // check selectrix channels for changes
  byte d=sx.get(SIGNAL_ADDR);
  val = d & 0x0f;
  val1 = (d >>4) & 0x0f;

  /*if (d != oldSX[0]) {
     // signal 0 = high Byte
     byte v = (d & 0x30) >> 4;
     sigs[0].set(v);
     
     // signal 1 = low Byte
     v = (d & 0x03) ;
     sigs[1].set(v);

     oldSX[0] = d;

  } */
  
  d=sx.get(TURNOUT_ADDR);
  if (d != oldSX[1]) {
    for (uint8_t i; i < 4; i++) {
      turnouts.set(i, bitRead(d, i) );
    }
    oldSX[1] = d;
  }
    
}



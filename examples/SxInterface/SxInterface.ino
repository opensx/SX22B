/*
 * sx_interface.ino
 *
 *  Created on: 10.11.2013
 *  Changed on: 10.12.2016
 *  Version:    2.2
 *  
 *  Author: Michael Blank
 *  
 *  Example program for the Selectrix(TM) Library
 *  
 *  1. sends SX data to serial port (only when data change)
 *  format:    F  80  16   (data bit 5 set on sx channel 80)
 *  
 *  2. and reads commands from the serial port, interprets
 *  them and send them on SX Bus.  
 *  format: (a)  S <ch> <data> \n  (or SET .. ..)
 *               S  80  255   \nsets all bits for channel 80 
 *          (b)  R <ch>    (read a channel) 
 *                   (response F <ch> <data>)
 *          (c)  X     (= read all channels)
 *                   (response F 1 <d1> \n
 *                             F 2 <d2> \n
 *                             ...
 *                             F 111 <d11> \n )
 *            
 *  All strings must be terminated by new-line character.
 *  If a correct command "S ..." is received, "OK" is answered
 *     if not "ERR" is the result.     
 *  
 */

#include <SX22b.h>   // this is the Selectrix library
#include <SX22Command.h>   // this is the Selectrix Command library

#define LED_PIN  13   // on most Arduinos there is an LED at pin 13

SX22b sx;                // library
SX22Command sxcmd;       // holds command data

static int ledState = LOW;
static byte oldSx[MAX_CHANNEL_NUMBER];

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void printSXValue(int i, int data) {
	// send data for 1 SX Channel on serial port
	Serial.print("F ");
	Serial.print(i);
	Serial.print(" ");
	Serial.println(data);
}

void toggleLed() {
	// to notify a change
	if (ledState == LOW) {
		ledState = HIGH;
	} else {
		ledState = LOW;
	}
	digitalWrite(LED_PIN, ledState);
}

void sxisr(void) {
	// if you want to understand this, see:
	// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239522239   
	sx.isr();
}

void setup() {
	inputString.reserve(80);
	pinMode(LED_PIN, OUTPUT);

	//digitalWrite(LED_PIN, ledState);
	Serial.begin(115200);      // open the serial port
	for (int i = 0; i < 112; i++) {
		oldSx[i] = 0;   // initialize to zero
		printSXValue(i, 0);
	}

	// initialize interrupt routine
	sx.init();

	// RISING slope on INT0 triggers the interrupt routine sxisr (see above)
	attachInterrupt(0, sxisr, RISING);
}

/*
 SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */

void serialEvent() {
	while (Serial.available()) {
		// get the new byte:
		char inChar = (char) Serial.read();
		// add it to the inputString:
		inputString += inChar;
		// if the incoming character is a newline, the command
		// was fully received.
		if (inChar == '\n') {
			stringComplete = true;
			if ((inputString[0] == 's') 
					|| (inputString[0] == 'S')) { // SET command
				sxcmd.decode(inputString);
				if (sxcmd.err == COMMAND_OK) {
					Serial.println("OK ");
					do {
						delay(10);
					} while (sx.set(sxcmd.channel, sxcmd.data));
				} else {
					Serial.println("ERR");
				}
			} else if ((inputString[0] == 'x') 
					|| (inputString[0] == 'X')) { // READ ALL command
				for (int i = 0; i < 112; i++) {
					byte d = sx.get(i);
					printSXValue(i, d);   // send new value to serial port
					oldSx[i] = d;
					printSXValue(i, d);   // send new value to serial port
				}
			    toggleLed();  // indicate change
			} else if ((inputString[0] == 'r') 
					|| (inputString[0] == 'R')) { // READ command
				sxcmd.decodeChannel(inputString);
				if (sxcmd.err == COMMAND_OK) {
					printSXValue(sxcmd.channel, sxcmd.data);   // send value to serial port
					toggleLed();  // 					
				} else {
					Serial.println("ERR");
				}
			} else {
				Serial.println("ERR");
			}
			//Serial.print("read=");
			//Serial.println(inputString);
			inputString = "";
			stringComplete = false;
		}
	}
}

void loop() {

	// check selectrix channels for changes
	for (int i = 0; i < 112; i++) {
		byte d = sx.get(i);
		if (oldSx[i] != d) {   // data have changed on SX bus
			oldSx[i] = d;
			printSXValue(i, d);   // send new value to serial port
			toggleLed();  // indicate change
		}
	}

}


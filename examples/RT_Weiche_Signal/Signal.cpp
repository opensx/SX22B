/*

 model a SINGLE signal with fading leds
 
 version for english signals, Common Ground (A2982)

 */

#include <Arduino.h> 
#include "signal_def.h"
#include "Signal.h"

// create a signal with 4 aspects
Signal::Signal(uint8_t n) {
	number = n;   // A=0, B=1
	mytimer = 0;
}

void Signal::init() {
	for (uint8_t i = 0; i < N_ASPECTS; i++) {
		final[i] = actual[i] = 0;  // off   
	}
	final[0] = actual[0] = 18;  // red = on
	// value gets written in interrupt routine
	state = 0;  // red

	if (number == 0) {
		pinMode(SIG_A_R, OUTPUT);
		pinMode(SIG_A_G, OUTPUT);
		pinMode(SIG_A_Y, OUTPUT);
		pinMode(SIG_A_4, OUTPUT);
	} else {
		pinMode(SIG_B_R, OUTPUT);
		pinMode(SIG_B_G, OUTPUT);
		pinMode(SIG_B_Y, OUTPUT);
		pinMode(SIG_B_4, OUTPUT);
	}

}

// version for english signals, Command Ground (A2982)
// value 0 = RED
// value 1 = GREEN
// value 2 = YELLOW
// value 3 = Feather & YELLOW

uint8_t Signal::set(uint8_t value) {
	switch (value) {
	case 0:
		final[0] = 16;
		final[1] = 0;
		final[2] = 0;
		final[3] = 0;
		state = 0;
		break;
	case 1:
		final[0] = 0;
		final[1] = 16;
		final[2] = 0;
		final[3] = 0;
		state = 1;
		break;
	case 2:
		final[0] = 0;
		final[1] = 0;
		final[2] = 16;
		final[3] = 0;
		state = 2;
		break;
	case 3:
		final[0] = 0;
		final[1] = 0;
		final[2] = 16;
		final[3] = 16;
		state = 3;
		break;
	default:
		// state unchanged
		return INVALID;
	}
	return state;  // new state
}

void Signal::process(void) {
	
	if ((millis() - mytimer) < 25)
		return;

	mytimer = millis();
	// check for dimming LEDs (up/down), analog value range 0..255
	for (uint8_t i = 0; i < N_ASPECTS; i++) {
		if (actual[i] < final[i]) {
			// increment 
			uint8_t intens = actual[i] + 2;
			if (intens > 16) {
				intens = 16;
			}
			actual[i] = intens;
			// value gets written in interrupt routine
		}
		if (actual[i] > final[i]) {
			// decrement
			uint8_t intens = actual[i] - 2;
			if (intens < 0) {
				intens = 0;
			}
			actual[i] = intens;
			// value gets written in interrupt routine
		}
	}
}


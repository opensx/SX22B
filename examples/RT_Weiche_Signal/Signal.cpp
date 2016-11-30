/*

model a SINGLE signal with fading leds

*/


#include <Arduino.h> 
#include "Signal.h"

// create a signal with 4 aspects
Signal::Signal(uint8_t r, uint8_t g, uint8_t y, uint8_t w) {
   // pins for signal (PWM) output
   _pin[0] = r;
   _pin[1] = g;
   _pin[2] = y;
   _pin[3] = w;
   _npins = 4;
}

// create a signal with 3 aspects
Signal::Signal(uint8_t r, uint8_t g, uint8_t y) {
   // pins for signal (PWM) output
   _pin[0] = r;
   _pin[1] = g;
   _pin[2] = y;
   _npins = 3;
}

// create a signal with 2 aspects
Signal::Signal(uint8_t r, uint8_t g) {
   // pins for signal (PWM) output
   _pin[0] = r;
   _pin[1] = g;
   _npins = 2;
}


void Signal::init() {
  for (int i =0; i <_npins; i++) {
     pinMode(_pin[i], OUTPUT);
     _final[i]=_actual[i]=0;  // off
     analogWrite(_pin[i], _actual[i]);
  }

  _final[0]=_actual[0]=255;  // red = on
  analogWrite(_pin[0], _actual[0]);
  _state=0;

}

uint8_t Signal::set(uint8_t value) {
    switch (value) {
    case 0: _final[0]=255;
            _final[1]=0;
            _final[2]=0;
            _final[3]=0;
            _state=0;
            break;
    case 1: _final[0]=0;
            _final[1]=255;
            _final[2]=0;
            _final[3]=0;
            _state=1;
            break;
    case 2: _final[0]=0;
            _final[1]=255;
            _final[2]=255;
            _final[3]=0;
            _state=2;
            break;
    case 3: _final[0]=0;
            _final[1]=0;
            _final[2]=0;
            _final[3]=255;
            _state=3;
            break;
    default:
            // state unchanged
            return INVALID;
    }
    return _state;  // new state
}

void Signal::process(void) {
    static long mytimer=0;
    if ((millis() - mytimer) < 50) return;

    mytimer = millis();
    // check for dimming LEDs (up/down), analog value range 0..255
    for (int i=0; i < _npins; i++) {
        if ( _actual[i] < _final[i] ) {
            // increment 
            int intens = _actual[i] + 50;
            if (intens > 255) {
                intens = 255;
            }
            _actual[i] = intens;
            analogWrite(_pin[i],intens);
        }
        if (_actual[i] > _final[i] ) {
            // decrement
            int intens = _actual[i] - 50;
            if (intens < 0) {
                intens = 0;
            }
            _actual[i] = intens;
            analogWrite(_pin[i],intens);
        }
    }
}

uint8_t Signal::get() {
   return _state;
}




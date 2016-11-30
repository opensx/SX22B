/* 

Signal.h

 *  Created on: 07,03.2016
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


#ifndef SIGNAL_H_
#define SIGNAL_H_
#include <WString.h>
#include <inttypes.h>

#define N_ASPECTS     4    // maximum 4 aspects
#define INVALID      99    // invalid value

class Signal {
   
public:
    Signal(uint8_t, uint8_t);  // 2 aspect signal red/green
	  Signal(uint8_t, uint8_t, uint8_t);  // 3 aspect signal red, green, (green+yellow)
    Signal(uint8_t, uint8_t, uint8_t, uint8_t);  // 4 aspect signal
           // red, green, (green+yellow), white (sh0)
    void process(void);  // needed for fade effects
    uint8_t set(uint8_t value);   // 0 ... N
    uint8_t get(void);   // return state
    void init(void);


private:
    uint8_t _actual[N_ASPECTS];  // actual (analog) value of pin
    uint8_t _final[N_ASPECTS];
    uint8_t _pin[N_ASPECTS];  // arduino pin number 
    uint8_t _npins;  // actual number of aspects for current signal
    uint8_t _state;  // contains last value of command = last known signal state
};

#endif /* SIGNAL_H_ */


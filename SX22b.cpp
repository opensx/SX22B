/*
 * SX22b.cpp
 *
 *  Created on: 02.01.2012
 *  Changed on: 10.11.2013
 *  Version:    2.1
 *
 *  Copyright:  Michael Blank
 *
 *  Changed on: 11.01.2015 by Reinhard Thamm, referring to comments
 *  Version:    2.2
 *
 *    This faster version uses more direct bitwise port and pin operations
 *    and needs to be used if an original TRIX ZENTRALEINHEIT 1 is in use.
 *    Only important for SX-Writing issues.
 *
 *
 *  Changed on: 09.05.2015 by Reinhard Thamm
 *  Version:    2.2b
 *
 *    Due to a timing problem when writing the very first Bit0 of a
 *    SX-address on the SX-bus, the handling of the SCOPE-trigger-
 *    signal has been postponed. The SCOPE-trigger is now activated
 *    after writing Bit0.
 *    
 *    24.09.2015: some cosmetic improvements.
 *
 *  interface hardware needed ! see www.oscale.net/SX

 Read SX Signal - SX Clock must be connected to Pin2=INT0 and
 SX Data must be connected to Pin4. For triggering a scope, a signal
 can be generated at Pin3 at a defined base address.

 (Uses digitalRead() function for maximum portability - is not optimized
 for speed of ISR routine!)

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

#include "SX22b.h"


SX::SX() {
    _scopeFlag = 0;
}

void SX::init() {
     // initialize function
     // initialize pins and variables
     // and start looking for a SYNC signal

    pinMode(SX_CLK, INPUT);      // SX-clock is an input
    pinMode(SX_DATA,INPUT);      // SX-data is also an input
    //digitalWrite(SX_CLK, HIGH);  // = enable pullup resistor
    //digitalWrite(SX_DATA, HIGH); // = enable pullup resistor

    for (int i=0; i<112;i++) {
          // reset sx variable to zero
        _sx[i]=0;
    }
     _toggle=0;
    _adrCount =0;

    // start always with search for SYNC
    _state = SYNC;
    _zeroCount = 0;
    _sx_write_busy = 0;                                             //Tha
    _sx_writing = 0;                                                //Tha
    _newTrackBit = 2;                                               //Tha
}

void SX::init(uint8_t tba) {
    // special init which enable a scope trigger
    // at a Selectrix address tba (must be 0..112)
    // scope will go high at bit 12 of this address!!
    // i.e. interesting data are shortly BEFORE
    // this trigger
    _scopeFlag=1;
     pinMode(SCOPE,OUTPUT);
    _triggerAdr = tba;
    init();
}

uint8_t SX::set(uint8_t ch, uint8_t d) {
    if (_sx_write_busy) return 1;  // dont accept new data

    _sx_write_channel = ch;
    _sx_write_data = d;
    _sx_write_busy=1;
    return 0;

}

void SX::switchAdr() {
     // a SYNC signal was received, now look for a valid
     // base address
    switch(_adrCount) {
    case 0:   // this is the GLEISSPANNUNG bit              //Tha: next 10 loc due to an idea from
        _trackBit = _bit;                                   //Tha    Uli Beyenbach
        if (_newTrackBit != 2) {                            //Tha
            bitWrite(SX_OUT_DDR, SX_OUT_PORTPIN, HIGH);     //Tha:   setting to HIGH as OUTPUT
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, _newTrackBit); //  and write the newTrackBit
        }                                                   //Tha
        break;                                              //Tha
    case 1:
        if (_newTrackBit != 2) {                            //Tha
            _newTrackBit = 2;                               //Tha
            bitWrite(SX_OUT_DDR, SX_OUT_PORTPIN, LOW);      //Tha;   setting to LOW as INPUT
        }
    case 4:
        break; // ignore
    case 2:  // B3
        bitWrite(_baseAdr,3,_bit);
        break;
    case 3:  // B2
        bitWrite(_baseAdr,2,_bit);
        break;
    case 5:  // B1
        bitWrite(_baseAdr,1,_bit);
        break;
    case 6:  // B0
        bitWrite(_baseAdr,0,_bit);
        break;
    case 7: // last "1"
        // _baseAdr is complete !

        // advance to next state - next we are looking
        // for the 7 data bytes (i.e. 7 SX Channels)
        _state = DATA;
        _dataFrameCount = 0;
        _dataBitCount = 0;
        _data=0;
        break;
    }
}

void SX::switchData() {
    // continue reading _data
    // a total of 7 DATA blocks will be received
    // for a certain base-address
    // calc sx channel from baseAdr and dataFrameCount
    _channel = (15-_baseAdr) + ((6-_dataFrameCount)<<4);

    switch(_dataBitCount) {
    case 2:  // "Trenn_bits"
    case 5:
    case 8:
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, HIGH);    //Tha:	digitalWrite(SX_OUT, HIGH);
        }
        _dataBitCount++;
        break; // ignore
    case 0:  // D0
        _data=0;

        // initiate write if requested
        if ((_sx_write_busy) && (_channel == _sx_write_channel)) {  //Tha:  next 2 loc changed (reverse
                                                                    //      order)
            bitWrite(SX_OUT_DDR, SX_OUT_PORTPIN, HIGH);             //Tha:  pinMode(SX_OUT,OUTPUT);

            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, bitRead(_sx_write_data, 0));
                                                                    //Tha: digitalWrite(SX_OUT, ...);

            //****************** start writing 1 byte to SX bus
            _sx_writing=1;
        }
        else {
            bitWrite(SX_OUT_DDR, SX_OUT_PORTPIN, LOW);              //Tha:  pinMode(SX_OUT,INPUT)

            _sx_writing=0; //defensive programming...
        }
        bitWrite(_data,0,_bit);
        _dataBitCount++;

        if (_scopeFlag) {                                           //Tha: also watching _scopeFlag..
            if (_channel ==  _triggerAdr) {                         //     (bugfix 30.01.2015)
                bitWrite(SCOPE_PORT, SCOPE_PORTPIN, HIGH);          //Tha: digitalWrite(SCOPE,HIGH);
            }
            else {
                bitWrite(SCOPE_PORT, SCOPE_PORTPIN, LOW);           //Tha: digitalWrite(SCOPE,LOW);
            }
        }

        break;
    case 1:  // D1
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, bitRead(_sx_write_data, 1));
        }                                                           //Tha: digitalWrite(SX_OUT, ...);
        bitWrite(_data,1,_bit);
        _dataBitCount++;
        break;
    case 3:  // D2
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, bitRead(_sx_write_data, 2));
        }                                                           //Tha: digitalWrite(SX_OUT, ...);
        bitWrite(_data,2,_bit);
        _dataBitCount++;
        break;
    case 4:  // D3
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, bitRead(_sx_write_data, 3));
        }                                                           //Tha: digitalWrite(SX_OUT, ...);
        bitWrite(_data,3,_bit);
        _dataBitCount++;
        break;
    case 6:  // D4
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, bitRead(_sx_write_data, 4));
        }                                                           //Tha: digitalWrite(SX_OUT, ...);
        bitWrite(_data,4,_bit);
        _dataBitCount++;
        break;
    case 7:  // D5
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, bitRead(_sx_write_data, 5));
        }                                                           //Tha: digitalWrite(SX_OUT, ...);
        bitWrite(_data,5,_bit);
        _dataBitCount++;
        break;
    case 9:  // D6
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, bitRead(_sx_write_data, 6));
        }                                                           //Tha: digitalWrite(SX_OUT, ...);
        bitWrite(_data,6,_bit);
        _dataBitCount++;
        break;
    case 10: // D7
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, bitRead(_sx_write_data, 7));
        }                                                           //Tha: digitalWrite(SX_OUT, ...);
        bitWrite(_data,7,_bit);
        _dataBitCount++;
        break;
    case 11:  // == MAX_DATABITCOUNT
        // _bit value should always equal HIGH, not tested here.
        if (_sx_writing)  {
            bitWrite(SX_OUT_PORT, SX_OUT_PORTPIN, HIGH);            //Tha: writing HIGH (Trenn-Bit)

            bitWrite(SX_OUT_DDR, SX_OUT_PORTPIN, LOW);              //Tha:  pinMode(SX_OUT, INPUT)
                                                                    //      setting to LOW as INPUT
                                                                    //      (high impedance)

            _sx_writing = 0; // was the last bit
            _sx_write_busy = 0;   // write has ended, can accept new data
        }
        // copy _data byte to SX _channel
        _sx[_channel] = _data;

        // increment dataFrameCount to move on the next DATA byte
        // check, if we already reached the last DATA block - in this
        // case move on to the next SX-Datenpaket, i.e. look for SYNC
        _dataFrameCount ++;
        if (_dataFrameCount == MAX_DATACOUNT) {
            // and move on to SYNC _state
            _dataFrameCount=0;
            _state =SYNC;
            _zeroCount = 0;
            _dataBitCount=0;
        } else {
            _dataBitCount = 0;  // reset _bit counter
            _data = 0;
            // continue with reading next _data uint8_t
        }
    }  //end switch/case _dataBitCount
}

uint8_t SX::get(uint8_t channel) {
     // returns the value of a SX channel
    if (channel < MAX_CHANNEL_NUMBER)
       return _sx[channel];
    else
       return 0;
}

uint8_t SX::getTrackBit() {                                     //Tha: getting the Track-Bit (track
    return _trackBit;                                           //      voltage on/off), due to an
}                                                               //      idea from Uli Beyenbach

void SX::setTrackBit(uint8_t val) {                             //Tha: setting the Track-Bit (track
    if (val==0 || val==1) {                                     //      voltage on/off), due to an
        _newTrackBit = val;                                     //      idea from Uli Beyenbach
    }
}

uint8_t SX::writing() {                                         //Tha:
    return _sx_write_busy;                                      //
}

void SX::isr() {

    // interrupt service routine (AVR INT0)
    // driven by RISING clock signal T0 (SX pin 1)

     // 3 different states are distinguished
     //     1. SNYC = looking for a SYNC signal
     //     2. ADDR = (after SYNC received) look for base address (0..15)
     //     3. DATA = (after ADDR decoded) decode the 7 data-bytes

    _bit = (SX_DATA_PINREG & _BV(SX_DATA_PORTPIN))>0;           //Tha: _bit = digitalRead(SX_DATA);

    switch(_state) {
    case SYNC:

        if (_bit == LOW)  {
            _zeroCount++;
        } else {
            if (_zeroCount == 3)  {        // sync bits 0 0 0 1 found
                _state = ADDR;         // advance to next state
                _baseAdr = 0;   //init
                _adrCount = 0;  //init
            } else {  // no valid sync, try again ...
                _zeroCount = 0;       // reset _zeroCounter
            } // endif _zeroCount
        }  // endif _bit==LOW
        break;
    case ADDR:
        switchAdr();
        _adrCount++;
        break;
    case DATA:
        switchData();
    }
}





/* 

 signal_def.h


 *  Created on: 30 nov 2016
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


#ifndef SXPIN_DEF_H_
#define SXPIN_DEF_H_
#include <WString.h>
#include <inttypes.h>

// for 8x 5 Volt outputs, Common Ground
// 0 Ohm resistors instead of A2982

//for faster direct bitwise operations

#define SX_BIT8           A0   
#define SX_BIT8_DDR       DDRC      
#define SX_BIT8_PORT      PORTC           
#define SX_BIT8_PPIN      PORTC0  

#define SX_BIT7           12   
#define SX_BIT7_DDR       DDRB      
#define SX_BIT7_PORT      PORTB           
#define SX_BIT7_PPIN      PORTB4    

#define SX_BIT6           11   
#define SX_BIT6_DDR       DDRB      
#define SX_BIT6_PORT      PORTB           
#define SX_BIT6_PPIN      PORTB3   

#define SX_BIT5           10   
#define SX_BIT5_DDR       DDRB      
#define SX_BIT5_PORT      PORTB           
#define SX_BIT5_PPIN      PORTB2   

#define SX_BIT8_(val)   (bitWrite(SX_BIT8_PORT, SX_BIT8_PPIN, val)) 
#define SX_BIT7_(val)   (bitWrite(SX_BIT7_PORT, SX_BIT7_PPIN, val)) 
#define SX_BIT6_(val)   (bitWrite(SX_BIT6_PORT, SX_BIT6_PPIN, val)) 
#define SX_BIT5_(val)   (bitWrite(SX_BIT5_PORT, SX_BIT5_PPIN, val)) 

#define SX_BIT4           9   
#define SX_BIT4_DDR       DDRB      
#define SX_BIT4_PORT      PORTB           
#define SX_BIT4_PPIN      PORTB1  

#define SX_BIT3           8   
#define SX_BIT3_DDR       DDRB      
#define SX_BIT3_PORT      PORTB           
#define SX_BIT3_PPIN      PORTB0    

#define SX_BIT2           7   
#define SX_BIT2_DDR       DDRD      
#define SX_BIT2_PORT      PORTD           
#define SX_BIT2_PPIN      PORTD7   

#define SX_BIT1           5   
#define SX_BIT1_DDR       DDRD      
#define SX_BIT1_PORT      PORTD           
#define SX_BIT1_PPIN      PORTD5   

#define SX_BIT4_(val)   (bitWrite(SX_BIT4_PORT, SX_BIT4_PPIN, val)) 
#define SX_BIT3_(val)   (bitWrite(SX_BIT3_PORT, SX_BIT3_PPIN, val)) 
#define SX_BIT2_(val)   (bitWrite(SX_BIT2_PORT, SX_BIT2_PPIN, val)) 
#define SX_BIT1_(val)   (bitWrite(SX_BIT1_PORT, SX_BIT1_PPIN, val)) 

#endif /* SXPIN_DEF_H_ */


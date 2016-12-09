#ifndef _HW_CONFIG_H
#define _HW_CONFIG_H


#define HARDWARE   "GP_OUT_8"    // 2 signals and 4 turnouts
// Platine E863820, Common Ground, direct output from ATmega328 used
//  R = 0 Ohm

#define DECODER_ADDR  (88)   // 
#define N_CHAN        (8)    // 8 output channels
#define MAX_ADDR     (108)   // maximum valid decoder address

// how to interpret sx programming values
#define SX_CHAN_ADDR   (1)   // SX address channel
#define EEPROM_ADDR    (1)   // eeprom address for storing decoder address



#endif

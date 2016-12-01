#ifndef _HW_CONFIG_H
#define _HW_CONFIG_H


#define HARDWARE   "RT_WEICHE_SIGNAL_0_1"    // 2 signals and 4 turnouts
// Platine E863820, Common Ground, A2982

// define two 4-aspect signals
#define DECODER_ADDR  (72)   // signals addresses = 72/bit7,8 and 72/bit 5,6
#define N_SIGNALS      (2)    // n= 2 signals
#define MAX_ADDR     (108)   // maximum valid decoder address

// how to interpret sx programming values
#define SX_CHAN_ADDR   (1)   // SX address channel
#define EEPROM_ADDR    (1)   // eeprom address for storing decoder address

// #define TURNOUT_ADDR (72)   // bits 1, 2, 3, 4

Turnouts turnouts(A4, A3, A2, A1);
uint8_t enablePin = 0;  // =RX

#endif

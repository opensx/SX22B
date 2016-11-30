#ifndef _HW_CONFIG_H
#define _HW_CONFIG_H


#define HARDWARE   "RT_WEICHE_SIGNAL_0_1"    // 2 signals and 4 turnouts
// Platine E863820

// define two 4-aspect signals
#define SIGNAL_ADDR  (72)   // signals addresses = 72/low 4 bits and 72/high 4 bits
#define N_SIGNALS     2   // maximum 2 signals

//Signal sigs[N_SIGNALS] = { Signal(A0,12,11,10), Signal(9,8,7,5) };

#define TURNOUT_ADDR (73)   // bits 1, 2, 3, 4
Turnouts turnouts(A4, A3, A2, A1);
uint8_t enablePin = 0;  // =RX


#endif

/*
 * SxXBee2.ino
 *  
 *  Changed on: 16.10.2015   corrected SX22Command
 *  Changed on: 12.10.2015   new protocol rev1.1
 *  Changed on: 04.09.2015   broadcast of power state
 *  Created on: 28.08.2015
 *  
 *
 *  Author and Copyright: Michael Blank
 *
 *  reads date from xbee and sends them to SX Bus
 *
 *  !!! SX lib does not work with SoftwareSerial Lib !!!
 *
 *  Hardware: SX-Basisplatine von R.Thamm 
 *     + Xbee an RX/TX des Arduino Pro Mini
 */


#include <SX22b.h>   // this is the Selectrix library
#include <SX22Command.h>   // this is the Selectrix Command library
#include <XBee.h> // XBee library by Andrew Rapp

#define LED  13   // LED indicator at pin 13

SX22b sx;                // selectrix library
SX22Command sxcmd;      // holds command data

static int ledState = LOW;
static byte oldSx[MAX_CHANNEL_NUMBER];
long lastPowerSent = 0;    // timer for repeating the power state messages

String inputString = "";         // a string to hold incoming data

static uint8_t track, oldTrack;

uint8_t payload[12] ;   // 12 byte string for sending data to xbees

// Create an XBee object
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();

// create reusable response objects for responses we expect to handle
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();

// Address of receiving XBee (always: Broadcast!)
XBeeAddress64 broadcast64;
ZBTxRequest zbTx;
ZBTxStatusResponse txStatus;


void sxisr(void) {
    // if you want to understand this, see:
    // http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239522239
    sx.isr();
}

void ledToggle() {
    static uint8_t ledState = LOW;
    if (ledState == HIGH) {
       ledState = LOW;
    } else {
       ledState = HIGH;
    }
    digitalWrite(LED, ledState);
}

/** init communication and XBee data package
*/
void setup() {

    Serial.begin(9600);      // for comm with xbee - TODO increase to 57600
    pinMode(LED,OUTPUT);     // indicator for rec. packet
    xbee.setSerial(Serial);
    broadcast64 = XBeeAddress64(0, 0x0000FFFF);  //broadcast address
    zbTx = ZBTxRequest(broadcast64, payload, sizeof(payload));  // 64bit address
    zbTx.setAddress16(0xFFFE); // addr16 for broadcast;
    txStatus = ZBTxStatusResponse();
  
    track = oldTrack = 0;

    // RISING slope on INT0 triggers the interrupt routine sxisr (see above)
    attachInterrupt(0, sxisr, RISING);

    delay(100); // give some time to initalize
}

/** transmit loco data as "adr data" broadcast
    to all xbees
*/
void sendData(uint8_t ch, uint8_t d) {
  for (int i=0; i <12; i++) {
    payload[i]=0;
  }
   payload[0]='F';  // for "feedback"
  // address (channel) to string
   uint8_t p3 = ch / 100;
   uint8_t p2 = (ch - 100*p3) / 10;
   uint8_t p1 = (ch - 100*p3 - 10*p2);
   uint8_t end = 3;
   
   if (p3 == 0) {
      payload[1]= (uint8_t)p2 + '0';
      payload[2]= (uint8_t)p1 + '0';
      payload[3]= ' ';
      end = 3;
   } else {
      payload[1]= (uint8_t)p3 + '0';
      payload[2]= (uint8_t)p2 + '0';
      payload[3]= (uint8_t)p1 + '0';
      payload[4]= ' ';
      end = 4;
   }

   // convert data to string
   p3 = d / 100;
   p2 = (d - 100*p3) / 10;
   p1 = (d - 100*p3 - 10*p2);
   if (p3 == 0) {
      payload[end+1]= (uint8_t)p2 + '0';
      payload[end+2]= (uint8_t)p1 + '0';
      payload[end+3]= '\n';
   } else {
      payload[end+1]= (uint8_t)p3 + '0';
      payload[end+2]= (uint8_t)p2 + '0';
      payload[end+3]= (uint8_t)p1 + '0';
      payload[end+4]= '\n';   // index max. 8 (of 11)
   }

#if defined(_DEBUG) || defined(_DEBUG_AVR)
   sendPayloadToSerial();
#endif   // defined(_DEBUG) || defined(_DEBUG_AVR)

#ifndef _DEBUG_AVR
   xbee.send(zbTx);
#endif
}

/** receive loop for:
 *     a) receiving data from XBee Throttles
 *     b) send (regularly) broadcast of power state to XBee Throttles
*/
void loop() {
   xbee.readPacket();
   if (xbee.getResponse().isAvailable()) {
      // got something
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
         digitalWrite(LED,HIGH);  // indicate start of received packet
         // got a zb rx packet
         // now fill our zb rx class
         xbee.getResponse().getZBRxResponse(rx);
         String s="";
         for (int i=0; i<rx.getDataLength(); i++) {
            char c = rx.getData(i);
            s += c;
         }
         if ((s[0] == 'R') || (s[0] == 'r')) {
            // read command, 1 int attribute
            // "R44" =read data of channel 44
            String ch = s.substring(1);
            int i_ch = ch.toInt();
            if ((i_ch >=1) && (i_ch <=99)) {
               uint8_t d = sx.get(i_ch);
               sendData((uint8_t)i_ch, d);
            } else if (i_ch == 127) {
               uint8_t d = sx.getTrackBit();
               sendData((uint8_t)i_ch, d);
            }
         } else if ((s[0] == 'S') || (s[0] == 's')) {
            // "SET" command
            // interpret SX command from XBee
            int n= s.indexOf(' '); // command split by space
            // example:  "S44 31" = set loco #44 to speed 31,
            //                forward, Light=off, F=off
            //       (standard selectrix data format, 8 bit)
            // 127=trackpower also handled like a standard SX channel
            if (n != -1) {
               String ch = s.substring(1,n); // remove leading 's', until space
               int i_ch = ch.toInt();
               String s2 = s.substring(n+1,s.length());
               int i_value = s2.toInt();
               if ((i_value >= 0 ) && (i_value <= 255)) {  //data valid
                  if (i_ch == 127) {
                    sx.setTrackBit(i_value);
                  } else {
                    uint8_t dsx = sx.get(i_ch);  // read current data on bus for this loco
                    if (dsx != (uint8_t)i_value) { // sent only if data have changed
                       do {
                          delay(10);
                       } while (sx.set(i_ch,i_value));  // send to SX bus
                    }
                  }
               }
            }

         }
         digitalWrite(LED,LOW);  // indicate stop of send to SX Bus
      }
   } /* else if (xbee.getResponse().isError()) {
      Serial.println("Error reading packet.  Error code: ");
CURRENTLY NOT HANDLED
   }  ******************************************************/

    // poll trackbit info and send as broadcast at least every 10 secs
    int track = sx.getTrackBit();
    if ((track != oldTrack) || ( (millis() - lastPowerSent) > 10000 ) ){  
      oldTrack = track;
      lastPowerSent = millis();
      if (track == 0) {
         sendData(127,0);
      } else {
         sendData(127,1);
      }
    }

}

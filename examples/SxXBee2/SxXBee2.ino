/*
 * SxXBee2.ino
 *
 *  Created on: 28.08.2015
 *  Changed on: 30.08.2015
 *  Changed on: 04.09.2015   broadcast of power state
 *
 *  Author and Copyright: Michael Blank
 *
 *  reads date from xbee and sends them to SX Bus
 *
 *  !!! SX lib does not work with SoftwareSerial Lib !!!
 *
 *  benötigte Hardware:
 *     SX-Basisplatine von R.Thamm 
 *     + Xbee an RX/TX des Arduino Pro Mini
 *  siehe http://opensx.net/funkregler/basisstation-xbee-platine
 */


#include <SX22.h>   // this is the Selectrix library
#include <SX22Command.h>   // this is the Selectrix Command library
#include <XBee.h> // XBee library by Andrew Rapp

#define LED  13   // LED indicator at pin 13

SX sx;                // selectrix library
SXCommand sxcmd;      // holds command data

static int ledState = LOW;
static byte oldSx[MAX_CHANNEL_NUMBER];
long lastPowerSent = 0;    // timer for repeating the power state messages

String inputString = "";         // a string to hold incoming data

static uint8_t track, oldTrack;

uint8_t payload[12] ;   // 12 byte string for sending data to coordinator

// Create an XBee object
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();

// create reusable response objects for responses we expect to handle
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();

// Address of receiving XBee
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

    Serial.begin(9600);      // for comm with xbee
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

/** transmit power state as broadcast to all XBees
 *  "P 1" = track power (gleisbit) is on
 *  "P 0" = track power is off
*/
void send_power_state(uint8_t on) {
  for (int i=0; i <12; i++) {
    payload[i]=0;
  }
      payload[0]='P';
      payload[1]= ' ';
      if (on == 1) {
         payload[2] = '1';
      } else {
         payload[2] = '0';
      }
      payload[3]=0;  // string end  */

      xbee.send(zbTx);
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

         // interpret SX command from XBee
         int n= s.indexOf(' '); // command split by space
         // example:  "44 31" = set loco #44 to speed 31,
         //                forward, Light=off, F=off
         //       (standard selectrix data format, 8 bit)
         if (n != -1) {
            String ch = s.substring(0,n);
            int i_ch = ch.toInt();
            String s2 = s.substring(n+1,s.length());
            int i_value = s2.toInt();
            do {
 		delay(10);
            } while (sx.set(i_ch,i_value));  // send to SX bus

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
      send_power_state(track);
    }

}

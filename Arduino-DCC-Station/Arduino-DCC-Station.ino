// This project is available on https://github.com/doyleLai/Arduino-DCC-Station
// The program code is adapted from Michael Blank's work. 

// 23. November 2009
// works well with LMD18200 Booster !!!!!

/* Copyright (C) 2009 Michael Blank
  This program is free software; you can redistribute it and/or modify it under the terms of the
  GNU General Public License as published by the Free Software Foundation;
  either version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
*/

#include "DCC_Controller.cpp"
#include "DCC_Packet_Generator.cpp"
#include "Msgs_Pool.cpp"

// DCC Output Pins: pin 11 and 12
#define DRIVE_1() PORTB = B00010000
#define DRIVE_0() PORTB = B00001000

//Timer frequency is 2MHz for ( /8 prescale from 16MHz )
#define TIMER_SHORT 0x8D  // 58usec pulse length 
#define TIMER_LONG  0x1B  // 116usec pulse length 

unsigned char last_timer = TIMER_SHORT; // store last timer value

unsigned char flag = 0; // used for short or long pulse
unsigned char every_second_isr = 0;  // pulse up or down

// definitions for state machine
#define PREAMBLE 0
#define SEPERATOR 1
#define SENDBYTE  2

unsigned char state = PREAMBLE;
unsigned char preamble_count = 16;
unsigned char outbyte = 0;
unsigned char cbit = 0x80;

Packet cachedMsg = {{ 0xFF, 0x00, 0xFF, 0, 0, 0, 0}, 3};
Msgs_Pool * msgsPool = new Msgs_Pool();
int byteIndex = 0;

DCC_Controller * dccController = new DCC_Controller(msgsPool);

//Setup Timer2.
//Configures the 8-Bit Timer2 to generate an interrupt at the specified frequency.
//Returns the time load value which must be loaded into TCNT2 inside your ISR routine.
void SetupTimer2() {

  //Timer2 Settings: Timer Prescaler /8, mode 0
  //Timmer clock = 16MHz/8 = 2MHz oder 0,5usec
  TCCR2A = 0;
  TCCR2B = 0 << CS22 | 1 << CS21 | 0 << CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 = 1 << TOIE2;

  //load the timer for its first cycle
  TCNT2 = TIMER_SHORT;

}

//Timer2 overflow interrupt vector handler
ISR(TIMER2_OVF_vect) {
  //Capture the current timer value TCTN2. This is how much error we have
  //due to interrupt latency and the work in this function
  //Reload the timer and correct for latency.
  // for more info, see http://www.uchobby.com/index.php/2007/11/24/arduino-interrupts/
  unsigned char latency;


  // for every second interupt just toggle signal
  if (every_second_isr)  {
    //digitalWrite(DCC_PIN,1);
    DRIVE_1();
    every_second_isr = 0;
   // if (preamble_count == 16) {
   //   PORTD = B11000000;
   // }

    // set timer to last value
    latency = TCNT2;
    TCNT2 = latency + last_timer;

  }  else  {  // != every second interrupt, advance bit or state
    //digitalWrite(DCC_PIN,0);
    DRIVE_0();
    every_second_isr = 1;

  //  if (preamble_count == 16) {
   //   PORTD = B00000000;
//}

    switch (state)  {
      case PREAMBLE:
        flag = 1; // short pulse
        preamble_count--;
        if (preamble_count == 0)  {  // advance to next state
          state = SEPERATOR;
          // get next message
          /*
          msgIndex++;
          if (msgIndex >= MAXMSG)  {
            msgIndex = 0;
          }
          */
          byteIndex = 0; //start msg with byte 0

          // testing code for Msgs Pool
          cachedMsg = msgsPool->getNextMsg();
          //Serial.println(msg2.data[0],HEX);
        }
        break;
      case SEPERATOR:
        flag = 0; // long pulse
        // then advance to next state
        state = SENDBYTE;
        // goto next byte ...
        cbit = 0x80;  // send this bit next time first (0x80 = 1000 0000)
        //outbyte = msg[msgIndex].data[byteIndex];
        outbyte = cachedMsg.data[byteIndex];
        break;
      case SENDBYTE:
        if (outbyte & cbit)  {
          flag = 1;  // send short pulse
        }  else  {
          flag = 0;  // send long pulse
        }
        cbit = cbit >> 1;
        if (cbit == 0)  {  // last bit sent, is there a next byte?
          byteIndex++;
          //if (byteIndex >= msg[msgIndex].len)  {
          if (byteIndex >= cachedMsg.len)  {
            // this was already the XOR byte then advance to preamble
            state = PREAMBLE;
            preamble_count = 16;
          }  else  {
            // send separtor and advance to next byte
            state = SEPERATOR ;
          }
        }
        break;
    }

    if (flag)  {  // if data==1 then short pulse
      latency = TCNT2;
      TCNT2 = latency + TIMER_SHORT;
      last_timer = TIMER_SHORT;
    }  else  {   // long pulse
      latency = TCNT2;
      TCNT2 = latency + TIMER_LONG;
      last_timer = TIMER_LONG;
    }
  }

}

void setup(void) {

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);

  SetupTimer2();
}


bool isStarted = false;
int inChar;
String inString = "";    // string to hold input

void loop(void) {
  //delay(1);
  while (Serial.available()) {
    inChar  = Serial.read();
    //Serial.println("R");
    if(inChar == '<'){
      isStarted = true;
      inString = "";
    }
    else if (inChar == '>'){
      Serial.println(inString);   
      dccController->processFrame(inString);
      isStarted = false;
    }
    else if (isStarted){
      inString += (char)inChar;
    }
  }
}

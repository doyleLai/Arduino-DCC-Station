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
//#include "Decoder.cpp"
//#include "Message.cpp"

#include "DCC_Controller.cpp"
#include "DCC_Packet_Generator.cpp"
#include "Msgs_Pool.cpp"

// DCC Output Pins: pin 11 and 12
#define DRIVE_1() PORTB = B00010000
#define DRIVE_0() PORTB = B00001000

//#define DCC_PIN    11  // Arduino pin for DCC out 
// this pin is connected to "DIRECTION" of LMD18200
//#define DCC_PWM    5  // must be HIGH for signal out
// connected to "PWM in" of LMD18200
//#define DCC_THERM  7  // thermal warning PIN
//#define AN_SPEED   2  // analog reading for Speed Poti
//#define AN_CURRENT 0  // analog input for current sense reading


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

// variables for throttle

//int last_locoSpeed = 0;
//int last_dir;
//int dirPin = 9;
//int FPin[] = { 5,6,7,8};
//int maxF =3;
//int locoAdr = 0x03; // this is the (fixed) address of the loco


// buffer for command
/*
  struct Message {
  unsigned char data[7];
  unsigned char len;
  } ;
*/
//#define MAXMSG  10
// for the time being, use only two messages - the idle msg and the loco Speed msg

//struct Message msg[MAXMSG] = {
//  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
//  { { locoAdr, 0x3F,  0, 0, 0, 0, 0}, 4},   // locoMsg with 128 speed steps
//  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
//  { { locoAdr, 0x80,  0, 0, 0, 0, 0}, 3},    // locoMsg function group 1
//  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
//  { { locoAdr, 0xB0,  0, 0, 0, 0, 0}, 3},    // locoMsg function group 2-1 (1011)
//  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
//  { { locoAdr, 0xA0,  0, 0, 0, 0, 0}, 3},    // locoMsg function group 2-2 (1010)
//  { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
//  { { locoAdr, 0xDE,  0, 0, 0, 0, 0}, 4}    // locoMsg Feature Expansion F13-F20 (1101 1110)
//};               // loco msg must be filled later with speed and XOR data byte

/*
struct Message msg[MAXMSG] = {
  { { 0xFF, 0x00, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
  { { 0x01, 0xFF,  0, 0, 0, 0, 0}, 4},   // locoMsg with 128 speed steps
  { { 0xFF, 0x00, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
  { { 0x01, 0xFF,  0, 0, 0, 0, 0}, 3},    // locoMsg function group 1
  { { 0xFF, 0x00, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
  { { 0x01, 0xFF,  0, 0, 0, 0, 0}, 3},    // locoMsg function group 2-1 (1011)
  { { 0xFF, 0x00, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
  { { 0x01, 0xFF,  0, 0, 0, 0, 0}, 3},    // locoMsg function group 2-2 (1010)
  { { 0xFF, 0x00, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
  { { 0x01, 0xDE,  0, 0, 0, 0, 0}, 4}    // locoMsg Feature Expansion F13-F20 (1101 1110)
};               // loco msg must be filled later with speed and XOR data byte

*/
//int msgIndex = 0;
Packet cachedMsg = {{ 0xFF, 0x00, 0xFF, 0, 0, 0, 0}, 3};
Msgs_Pool * msgsPool = new Msgs_Pool();
int byteIndex = 0;

DCC_Controller * dccController = new DCC_Controller(msgsPool);

//int currentCommand = 0;
//bool isLightOn = false;


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


//Decoder ktt(0x03);

void setup(void) {

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  //Set the pins for DCC to "output".

  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  //pinMode(7, OUTPUT);
  //pinMode(DCC_PIN,OUTPUT);   // this is for the DCC Signal

  //pinMode(DCC_PWM,OUTPUT);   // will be kept high, PWM pin
  //digitalWrite(DCC_PWM,1);

  //pinMode(DCC_THERM, INPUT);
  //digitalWrite(DCC_THERM,1); //enable pull up

  //pinMode(dirPin, INPUT);
  //digitalWrite(dirPin, 1);  //enable pull-up resistor !!

  //for (int i=0 ; i<=maxF; i++){
  //   pinMode(FPin[i], INPUT);
  //   digitalWrite(FPin[i], 1);  //enable pull-up resistor
  //}

  //read_locoSpeed_etc();
  //assemble_dcc_msg();


  //Message m = DCC_Message_Generator::getSpeedMessage(ktt);
  //Message m2 = DCC_Message_Generator::getFunctionGroup1Message(ktt);
  //Message m3 = DCC_Message_Generator::getFunctionGroup2Message1(ktt);
  //Message m4 = DCC_Message_Generator::getFunctionGroup2Message2(ktt);
  //Message m5 = DCC_Message_Generator::getFeatureExpansionF13F20Message(ktt);

  //noInterrupts();
  //msg[1] = m;
  //msg[3] = m2;
  //msg[5] = m3;
  //msg[7] = m4;
  //msg[9] = m5;
  //interrupts();
  //Start the timer
  SetupTimer2();

  //dccController->processFrame("S31002");
}

//int locoSpeed = 0;
//int dir;
bool isStarted = false;
//bool ended = false;
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
      dccController->processFrame(inString);
      isStarted = false;
      //ended = false;
    }
    else if (isStarted){
      inString += (char)inChar;
    }
  }

/*
  if (ended) {
    dccController->processFrame(inString);
    inString = "";
    isStarted = false;
    ended = false;
  }
*/
  /*
    if (read_locoSpeed_etc())  {
     // some reading changed
     // make new dcc message
     assemble_dcc_msg();
     ;
    }
  */
}

/*
void CmdChangeSpeed(bool dir, int speedStep) {
  ktt.dir = dir;
  ktt.speedStep = speedStep;
  Message m = DCC_Message_Generator::getSpeedMessage(ktt);
  noInterrupts();
  //msg[1] = m;
  interrupts();
}

void CmdToggleFunction(int fun){
  if (fun <= 20)  {
    ktt.ToggleFunc(fun);

    Message m;
    int index = 0;

    if (fun <= 4){
      m = DCC_Message_Generator::getFunctionGroup1Message(ktt);
      index = 3;
    }
    else if (fun <= 8){
      m = DCC_Message_Generator::getFunctionGroup2Message1(ktt);
      index = 5;
    }
    else if (fun <= 12){
      m = DCC_Message_Generator::getFunctionGroup2Message2(ktt);
      index = 7;
    }
    else if (fun <= 20){
      m = DCC_Message_Generator::getFeatureExpansionF13F20Message(ktt);
      index = 9;
    }

    noInterrupts();
    //msg[index] = m;
    interrupts();
  }
}
*/
/*
  void assemble_Function_Group1_msg(int fun) {
  // toggle F1 to F4 and FL
  unsigned char data = msg[3].data[1];
  unsigned char xdata;

  data ^= (0x01 << fun);
  Serial.println(data);
  xdata = msg[3].data[0] ^ data;

  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[3].data[1] = data;
  msg[3].data[2] = xdata;
  interrupts();
  }
*/

/*
  void assemble_Function_Group2_1_msg(int fun) {
  // toggle F5 to F8
  unsigned char data = msg[5].data[1];
  unsigned char xdata;

  data ^= (0x01 << (fun - 5));
  Serial.println(data);
  xdata = msg[5].data[0] ^ data;

  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[5].data[1] = data;
  msg[5].data[2] = xdata;
  interrupts();
  }
*/
/*
  void assemble_Function_Group2_2_msg(int fun) {
  // toggle F9 to F12
  unsigned char data = msg[7].data[1];
  unsigned char xdata;

  data ^= (0x01 << (fun - 9));
  Serial.println(data);
  xdata = msg[7].data[0] ^ data;

  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[7].data[1] = data;
  msg[7].data[2] = xdata;
  interrupts();
  }
*/
/*
  void assemble_Feature_Expansion_F13_F20_msg(int fun) {
  // toggle F13 to F20
  unsigned char data = msg[9].data[2];
  unsigned char xdata;

  data ^= (0x01 << (fun - 13));
  Serial.println(data);
  xdata = msg[9].data[0] ^ msg[9].data[1] ^ data;

  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[9].data[2] = data;
  msg[9].data[3] = xdata;
  interrupts();
  }
*/
/*
  void assemble_FL_msg(bool isOn) {
  unsigned char data;
  unsigned char xdata;
  if (isOn) {
    data = 0x90; //instr = 1001 0000
  }
  else {
    data = 0x80; //instr = 1000 0000
  }

  xdata = msg[3].data[0] ^ data;

  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[3].data[1] = data;
  msg[3].data[2] = xdata;
  interrupts();
  }*/

/*
  void assemble_dcc_msg() {
  int i;
  unsigned char data, xdata;

  if (locoSpeed == 1)  {  // this would result in emergency stop
    locoSpeed = 0;
  }

  // direction info first
  if (dir)  {  // forward
    data = 0x80;
  }  else  {
    data = 0;
  }

  data |=  locoSpeed;

  // add XOR byte
  xdata = (msg[1].data[0] ^ msg[1].data[1]) ^ data;

  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
  msg[1].data[2] = data;
  msg[1].data[3] = xdata;
  interrupts();

  }
*/

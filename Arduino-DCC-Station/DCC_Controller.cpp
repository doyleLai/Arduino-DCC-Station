// Part of the program code in this file is adapted from Michael Blank's work
// Which was written on 23 November 2009

#ifndef DCC_Controller_cpp
#define DCC_Controller_cpp

#include <Arduino.h>
#include "DCC_Controller.h"
#include "Packet.h"
#include "Msgs_Pool.h"

// DCC Output Pins: pin 11 and 12
#define DRIVE_1() PORTB = B00010000
#define DRIVE_0() PORTB = B00001000

// definitions for state machine
#define PREAMBLE 0
#define SEPERATOR 1
#define SENDBYTE  2

//Timer frequency is 2MHz for ( /8 prescale from 16MHz )
#define TIMER_SHORT 0x8D  // 58usec pulse length 
#define TIMER_LONG  0x1B  // 116usec pulse length 

unsigned char last_timer = TIMER_SHORT; // store last timer value

unsigned char flag = 0; // used for short or long pulse
unsigned char every_second_isr = 0;  // pulse up or down

unsigned char state = PREAMBLE;
unsigned char preamble_count = 16;
unsigned char outbyte = 0;
unsigned char cbit = 0x80;
int byteIndex = 0;

Packet cachedMsg = {{ 0xFF, 0x00, 0xFF, 0, 0, 0, 0}, 3};
//Msgs_Pool * msgsPool = new Msgs_Pool();
//DCC_Controller * dccController = new DCC_Controller(msgsPool);

//Setup Timer2.
//Configures the 8-Bit Timer2 to generate an interrupt at the specified frequency.
//Returns the time load value which must be loaded into TCNT2 inside your ISR routine.
static void SetupTimer2() {
  //Timer2 Settings: Timer Prescaler /8, mode 0
  //Timmer clock = 16MHz/8 = 2MHz oder 0,5usec
  TCCR2A = 0;
  TCCR2B = 0 << CS22 | 1 << CS21 | 0 << CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 = 1 << TOIE2;

  //load the timer for its first cycle
  TCNT2 = TIMER_SHORT;
}


DCC_Controller::DCC_Controller(){
 //  Serial.println("con");
 // SetupTimer2();
 // decoderCount = 0;
 // this->pool = new Msgs_Pool();
}


DCC_Controller::DCC_Controller(Msgs_Pool * pool){
  //Serial.println("con2");
  //SetupTimer2();
  //this->pool = pool;
}

void DCC_Controller::DCC_begin(){
  Serial.println("DCC_begin");
  SetupTimer2();
  decoderCount = 0;
  this->pool = new Msgs_Pool();
}

int DCC_Controller::getDecoderIndex(int address){
      for (int i = 0; i < decoderCount; i++){
        if(address == decoders[i].GetAddress()){
          return i;
        }
      }
      this->decoders[decoderCount] = Decoder(address);
      return decoderCount++;
}
/*
    Decoder * findDecoderByAddress(int address){
      for (int i = 0; i < decoderCount; i++){
        if(address == decoders[i].GetAddress()){
          return &decoders[i];
        }
      }
      return NULL;
    }

    Decoder * addDecoder(int address){
      // no duplicate checking. Make sure the new address does not exist.
      this->decoders[decoderCount] = Decoder(address);
      return &(this->decoders[decoderCount++]);
    }
*/
  // frame format: Type, Address, Payload
  // Address: 1 byte [#d1].
  // Type S (speed control), Payload is 1 byte of direction [0,1], followed by 3-byte fixed size with 128 speed steps [%d3].
  // Type F (Function control), Payload is function 2 byte index [%d2], followed by 1 byte of on/off [0,1].
  // Type E (Emergency Stop), Ignore Address and Payload.
  // Type R (Release Emergency stop)
  // Type C (Configure a CV value)

void DCC_Controller::processFrame(String frame){
      //Serial.println("Receieved Frame");
      char firstChar = frame[0];
      switch (firstChar){
        case 'S':
          CmdSpeed(frame);
          break;
        case 'F':
          CmdFunction(frame);
          break;
        case 'E':
          CmdEmergencyStop();
          break;
        case 'R':
          CmdRelease();
        case 'C':
          CmdChangeCV();
      }
}

void DCC_Controller::CmdSpeed(String frame){
      int address = frame.substring(1, 2).toInt();
      bool direction = frame.substring(2, 3).toInt();
      int speedStep = frame.substring(3, 6).toInt();

      //Serial.println("Address: " + String(address));
      //Serial.println("Direction: " + String(direction));
      //Serial.println("SpeedStep: " + String(speedStep));

      int decoderIndex = this->getDecoderIndex(address);

      Decoder * d = &decoders[decoderIndex];
      d->dir = direction;
      d->speedStep = speedStep;

      Packet m = DCC_Packet_Generator::getSpeedMessage(*d);

      pool->add(decoderIndex * 5 + m.type,m);
}
    
void DCC_Controller::CmdFunction(String frame){
      int address = frame.substring(1, 2).toInt();
      int fun = frame.substring(2, 4).toInt();
      bool isOn = frame.substring(4, 5).toInt();

      //Serial.println("Address: " + String(address));
      //Serial.println("Func: " + String(fun));
      //Serial.println("IsOn: " + String(isOn));

      int decoderIndex = this->getDecoderIndex(address);

      Decoder * d = &decoders[decoderIndex];
      if(isOn){
        d->SetFunc(fun);
      }
      else{
        d->ClearFunc(fun);
      }

      //Serial.println("Idebug" + d->toString());

      Packet m;
      if (fun <= 4){
        m = DCC_Packet_Generator::getFunctionGroup1Message(*d);
      }
      else if (fun <= 8){
        m = DCC_Packet_Generator::getFunctionGroup2Message1(*d);
      }
      else if (fun <= 12){
        m = DCC_Packet_Generator::getFunctionGroup2Message2(*d);
      }
      else if (fun <= 20){
        m = DCC_Packet_Generator::getFeatureExpansionF13F20Message(*d);
      }

      pool->add(decoderIndex * 5 + m.type,m);
}

void DCC_Controller::CmdEmergencyStop(){

}

void DCC_Controller::CmdRelease(){

}

void DCC_Controller::CmdChangeCV(){
      // you must reset the arduino after using this command.
      // Otherwise, the repeated instruction frame will interfere other frames.
      Packet m = DCC_Packet_Generator::getConfigurationVariableAccessInstructionPacket(0x03,1,0x05);
      pool->fill(m);
}

Packet DCC_Controller::getNextMsg(){
  return pool->getNextMsg();
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
    //digitalWrite(7, HIGH);
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
    // digitalWrite(7, LOW);
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
          cachedMsg = DCC.getNextMsg();
            //Serial.println(cachedMsg.data[0], HEX);     
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

DCC_Controller DCC;

#endif

// Part of the program code in this file is adapted from Michael Blank's work
// Which was written on 23 November 2009

#ifndef DCC_Controller_cpp
#define DCC_Controller_cpp

#include <Arduino.h>
#include "DCC_Controller.h"
#include "Packet.h"
#include "Packets_Pool.h"

static void SetupPins();
static inline void Drive0();
static inline void Drive1();
static void SetupTimer();

static void handle_interrupt(volatile uint8_t & TCNTx, volatile uint8_t & OCRx);

// Defines long pulse or short pulse should be sent.
typedef enum {
  Preamble, // short pulse
  Seperator, // long pulse
  SendByte // depends on the out bit.
} DCC_pulse_state_t;

#if defined(__AVR_ATmega328P__)

//Timer frequency is 2MHz for ( /8 prescale from 16MHz )
#define TIMER_SHORT 112  // 58usec pulse length
#define TIMER_LONG 196   // 100usec pulse length

// DCC Output Pins: pin 11 (PB.3) and 12 (PB.4)
static void SetupPins(){
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
}

static inline void Drive0(){
  PORTB |= (1 << 3);
  PORTB &= ~(1 << 4);
}

static inline void Drive1(){
  PORTB |= (1 << 4);
  PORTB &= ~(1 << 3);
}

//Setup Timer2.
//Configures the 8-Bit Timer2 to generate an interrupt at the specified frequency.
//Returns the time load value which must be loaded into TCNT2 inside your ISR routine.
static void SetupTimer() {
  // Normal Mode, OC2A OC2B disconnected. clk / 8 prescaler
  TCCR2A = 0x00; 
  TCCR2B = 0x00 | (1 << CS21); 
  TIMSK2 = 0x00 | (1 << OCIE2A); // Compare Match A interrupt enabled

  OCR2A = TIMER_SHORT;
  TCNT2 = 0;
}

ISR(TIMER2_COMPA_vect) {
  handle_interrupt(TCNT2, OCR2A);
}

#else
#error "Unsupported hardware"
#endif



static void handle_interrupt(volatile uint8_t & TCNTx, volatile uint8_t & OCRx){
  static DCC_pulse_state_t current_state = Preamble;
  static uint8_t timerValue = TIMER_SHORT;  // store last timer value
  //static unsigned char flag = 0;              // used for short or long pulse
  static bool isSecondPulse = false;
  static uint8_t preamble_count = 16;
  static unsigned char outbyte = 0;
  static unsigned char cbit = 0x80;
  static uint8_t byteIndex = 0;
  static Packet cachedMsg = DCC_Packet_Generator::getDigitalDecoderIdlePacket();
  static Packet resetPkt = DCC_Packet_Generator::getDigitalDecoderResetPacket();
  //static uint8_t latency;
  //Capture the current timer value TCTN2. This is how much error we have
  //due to interrupt latency and the work in this function
  //Reload the timer and correct for latency.
  // for more info, see http://www.uchobby.com/index.php/2007/11/24/arduino-interrupts/

  // for every second interupt just toggle signal
  if (isSecondPulse) {
    Drive1();
    isSecondPulse = false;
    // set timer to last value
    //latency = TCNTx;
    OCRx = TCNTx + timerValue;
    //TCNTx = latency + timerValue;
  
  } else {  // != every second interrupt, advance bit or state
    Drive0();
    isSecondPulse = true;

    switch (current_state) {
      case Preamble:
        timerValue = TIMER_SHORT; // This pulse is short 
        preamble_count--;
        if (preamble_count == 0) {  // advance to next state
          // preamble completed, get next message
          current_state = Seperator;
          byteIndex = 0;  //start msg with byte 0
          if (DCC.getSignalState()== SendPacket){
            cachedMsg = DCC.getNextPacket();
          }
          else{
            cachedMsg = resetPkt;
          }
        }
        break;
      case Seperator:
        timerValue = TIMER_LONG; // This pulse is long 
        // then advance to next state
        current_state = SendByte;
        // goto next byte ...
        cbit = 0x80;  // send this bit next time first (0x80 = 1000 0000)
        outbyte = cachedMsg.data[byteIndex];
        break;
      case SendByte:
        if (outbyte & cbit) {
          timerValue = TIMER_SHORT; // send short pulse
        } else {
          timerValue = TIMER_LONG; // send long pulse
        }
        cbit = cbit >> 1;
        if (cbit == 0) {  // last bit sent, is there a next byte?
          byteIndex++;
          if (byteIndex >= cachedMsg.len) {
            // this was already the XOR byte then advance to preamble
            current_state = Preamble;
            preamble_count = 16;
          } else {
            // send separtor and advance to next byte
            current_state = Seperator;
          }
        }
        break;
    }
    
    //latency = TCNTx;
    //TCNTx = latency + timerValue;
    OCRx = TCNTx + timerValue;
  }
}


DCC_Controller::DCC_Controller() {
  this->pool = new Packets_Pool();
  decoderCount = 0;
  signal_state = Startup;
}

/*
DCC_Controller::DCC_Controller(Msgs_Pool * pool){
  //Serial.println("con2");
  //SetupTimer2();
  //this->pool = pool;
}
*/
void DCC_Controller::DCC_begin() {

  SetupPins();
  SetupTimer();
  Serial.println("DCC_begin");
}

uint8_t DCC_Controller::getDecoderIndex(uint8_t address) {
  for (uint8_t i = 0; i < decoderCount; i++) {
    if (address == decoders[i].GetAddress()) {
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

// Returns true if the frame is valid, false otherwise.
bool DCC_Controller::processCommand(String frame) {
  //Serial.println("Receieved Frame");
  char firstChar = frame[0];
  bool isValid;
  switch (firstChar) {
    case 'S':
      isValid = CmdSpeed(frame);
      break;
    case 'F':
      isValid = CmdFunction(frame);
      break;
    case 'E':
      isValid = CmdEmergencyStop();
      break;
    case 'R':
      isValid = CmdRelease();
    case 'C':
      isValid = CmdChangeCV();
  }
  if (signal_state == Startup){
    signal_state = SendPacket;
  }
  return isValid;
}

bool DCC_Controller::CmdSpeed(String frame) {
  if (frame.length()!= 6){
    return false;
  }

  uint8_t address = frame.substring(1, 2).toInt();
  bool direction = frame.substring(2, 3).toInt();
  uint8_t speedStep = frame.substring(3, 6).toInt();

  //Serial.println("Address: " + String(address));
  //Serial.println("Direction: " + String(direction));
  //Serial.println("SpeedStep: " + String(speedStep));

  uint8_t decoderIndex = this->getDecoderIndex(address);

  Decoder* d = &decoders[decoderIndex];
  d->dir = direction;
  d->speedStep = speedStep;

  Packet m = DCC_Packet_Generator::getSpeedPacket(*d);

  pool->add(decoderIndex * 5 + m.type, m);

  return true;
}

bool DCC_Controller::CmdFunction(String frame) {
    if (frame.length()!= 5){
    return false;
  }

  uint8_t address = frame.substring(1, 2).toInt();
  uint8_t fun = frame.substring(2, 4).toInt();
  bool isOn = frame.substring(4, 5).toInt();

  //Serial.println("Address: " + String(address));
  //Serial.println("Func: " + String(fun));
  //Serial.println("IsOn: " + String(isOn));

  uint8_t decoderIndex = this->getDecoderIndex(address);

  Decoder* d = &decoders[decoderIndex];
  if (isOn) {
    d->SetFunc(fun);
  } else {
    d->ClearFunc(fun);
  }

  //Serial.println("Idebug" + d->toString());

  Packet m;
  if (fun <= 4) {
    m = DCC_Packet_Generator::getFunctionGroup1Packet(*d);
  } else if (fun <= 8) {
    m = DCC_Packet_Generator::getFunctionGroup2_1Packet(*d);
  } else if (fun <= 12) {
    m = DCC_Packet_Generator::getFunctionGroup2_2Packet(*d);
  } else if (fun <= 20) {
    m = DCC_Packet_Generator::getFeatureExpansionF13F20Packet(*d);
  }

  pool->add(decoderIndex * 5 + m.type, m);
  
  return true;
}

bool DCC_Controller::CmdEmergencyStop() {
  return false;
}

bool DCC_Controller::CmdRelease() {
  return false;
}

bool DCC_Controller::CmdChangeCV() {
  // you must reset the arduino after using this command.
  // Otherwise, the repeated instruction frame will interfere other frames.
  Packet m = DCC_Packet_Generator::getConfigurationVariableAccessInstructionPacket(0x03, 1, 0x05);
  pool->fill(m);
  return true;
}

DCC_signal_state_t DCC_Controller::getSignalState(){
  return signal_state;
}

Packet DCC_Controller::getNextPacket() {
  return pool->getNextPacket();
}

DCC_Controller DCC;

#endif

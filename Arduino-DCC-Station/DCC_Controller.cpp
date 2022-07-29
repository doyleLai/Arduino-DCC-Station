#ifndef DCC_Controller_cpp
#define DCC_Controller_cpp

#include <Arduino.h>
#include "DCC_Controller.h"
#include "DCC_Signal.h"
#include "Packet.h"
#include "Packets_Pool.h"

#if defined(__AVR__)
#include <avr/wdt.h>
#endif

DCC_Controller::DCC_Controller() {
  this->pool = Packets_Pool();
  this->decoderCount = (uint8_t) 0;
  this->resetPkt = DCC_Packet_Generator::getDigitalDecoderResetPacket();
  this->stopPkt = DCC_Packet_Generator::getDigitalDecoderBroadcastStopPacket();
  this->control_state = Startup;
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
// Message format: Type, Address, Payload
// Address: 1 byte [#d1].
// Type S (speed control), Payload is 1 byte of direction [0,1], followed by 3-byte fixed size with 128 speed steps [%d3].
// Type F (Function control), Payload is function 2 byte index [%d2], followed by 1 byte of on/off [0,1].
// Type E (Emergency Stop), Ignore Address and Payload.
// Type R (Release Emergency stop)
// Type C (Configure a CV value)

// Returns true if the message is valid, false otherwise.
bool DCC_Controller::processCommand(char msg[]) {
  //Serial.println("Receieved Frame");
  char firstChar = msg[0];
  bool isValid = false;
  switch (firstChar) {
    case 'S':
      isValid = CmdSpeed(msg);
      break;
    case 'F':
      isValid = CmdFunction(msg);
      break;
    case 'E':
      isValid = CmdEmergencyStop(msg);
      break;
    case 'R':
      isValid = CmdRelease(msg);
      break;
    case 'C':
      //isValid = CmdChangeCV();
      break;
    case 'W':
      isValid = CmdReset(msg);
      break;
  }
  if (control_state == Startup){
    cli();
    control_state = SendPacket;
    sei();
  }
  return isValid;
}

bool DCC_Controller::CmdSpeed(char msg[]) {
  if (strlen(msg) != 6){
    return false;
  }

  char address_text[2]; // Needs 1 more byte for '\0'
  char direction_text[2];
  char speedStep_text[4];

  strncpy(address_text, &msg[1], 1);
  address_text[1] = '\0';
  strncpy(direction_text, &msg[2], 1);
  direction_text[1] = '\0';
  strncpy(speedStep_text, &msg[3], 3);
  speedStep_text[3] = '\0';

  uint8_t address = (uint8_t) atoi(address_text);
  bool direction = (bool) atoi(direction_text);
  uint8_t speedStep = (uint8_t) atoi(speedStep_text);

  uint8_t decoderIndex = this->getDecoderIndex(address);

  Decoder* d = &decoders[decoderIndex];
  d->dir = direction;
  d->speedStep = speedStep;

  Packet m = DCC_Packet_Generator::getSpeedPacket(*d);

  pool.add(decoderIndex, m);

  return true;
}

bool DCC_Controller::CmdFunction(char msg[]) {
    if (strlen(msg) != 5){
    return false;
  }

  char address_text[2]; // Needs 1 more byte for '\0'
  char fun_text[3];
  char isOn_text[2];

  strncpy(address_text, &msg[1], 1);
  address_text[1] = '\0';
  strncpy(fun_text, &msg[2], 2);
  fun_text[2] = '\0';
  strncpy(isOn_text, &msg[4], 1);
  isOn_text[1] = '\0';

  uint8_t address = (uint8_t) atoi(address_text);
  uint8_t fun = (uint8_t) atoi(fun_text);
  bool isOn  = (bool) atoi(isOn_text);

  uint8_t decoderIndex = this->getDecoderIndex(address);

  Decoder* d = &decoders[decoderIndex];
  if (isOn) {
    d->SetFunc(fun);
  } else {
    d->ClearFunc(fun);
  }

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

  pool.add(decoderIndex, m);
  
  return true;
}

bool DCC_Controller::CmdEmergencyStop(char msg[]) {
  if (this->control_state!=EmergencyStop){
    // The signal generator will send stop packet only in EmergencyStop state.
    cli();
    this->control_state = EmergencyStop;
    sei();
    // Set the speed steps of all decoders in packet pool to 0
    // so trains will remain stationary after releasing EmergencyStop
    for (uint8_t i = 0; i < decoderCount; i++) {
      Decoder* _d = &this->decoders[i];
      _d->speedStep = 0;
      Packet m = DCC_Packet_Generator::getSpeedPacket(*_d);
      pool.add(i, m);
    }
    return true;
  }
  return false;
}

bool DCC_Controller::CmdRelease(char msg[]) {
  // Release from EmergencyStop state.
  if (this->control_state == EmergencyStop){
    cli();
    this->control_state = SendPacket;
    sei();
    return true;
  }
  return false;
}

bool DCC_Controller::CmdChangeCV(char msg[]) {
  // you must reset the arduino after using this command.
  // Otherwise, the CV instruction packet will stay in the pool forever.
  Packet m = DCC_Packet_Generator::getConfigurationVariableAccessInstructionPacket(0x03, 1, 0x05);
  pool.fill(m);
  return true;
}

bool DCC_Controller::CmdReset(char msg[]){
#if defined(__AVR__)
  wdt_enable(WDTO_15MS);
  return true;
#endif
  return false;
}

/*
DCC_control_state_t DCC_Controller::getControlState(){
  return control_state;
}
*/
Packet DCC_Controller::getNextPacket() {
  if (this->control_state == SendPacket){
    return pool.getNextPacket();
  }
  else if (this->control_state == EmergencyStop){
    return this->stopPkt;
  }
  else{
    return this->resetPkt;
  }
  
}

DCC_Controller DCC;

#endif

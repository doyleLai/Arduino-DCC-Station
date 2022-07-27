#ifndef DCC_Controller_cpp
#define DCC_Controller_cpp

#include <Arduino.h>
#include "DCC_Controller.h"
#include "DCC_Signal.h"
#include "Packet.h"
#include "Packets_Pool.h"


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
      break;
    case 'C':
      //isValid = CmdChangeCV();
      break;
  }
  if (control_state == Startup){
    cli();
    control_state = SendPacket;
    sei();
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

  pool.add(decoderIndex, m);

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

  pool.add(decoderIndex, m);
  
  return true;
}

bool DCC_Controller::CmdEmergencyStop() {
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

bool DCC_Controller::CmdRelease() {
  // Release from EmergencyStop state.
  if (this->control_state == EmergencyStop){
    cli();
    this->control_state = SendPacket;
    sei();
    return true;
  }
  return false;
}

bool DCC_Controller::CmdChangeCV() {
  // you must reset the arduino after using this command.
  // Otherwise, the CV instruction packet will stay in the pool forever.
  Packet m = DCC_Packet_Generator::getConfigurationVariableAccessInstructionPacket(0x03, 1, 0x05);
  pool.fill(m);
  return true;
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

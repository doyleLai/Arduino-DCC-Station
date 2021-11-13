#ifndef DCC_Controller_cpp
#define DCC_Controller_cpp

#include "Msgs_Pool.cpp"
#include "Decoder.cpp"

#define MAX_DECODERS 10

class DCC_Controller{
  private:
    Msgs_Pool * pool;
    Decoder decoders[MAX_DECODERS];
    int decoderCount = 0;

  public:
    DCC_Controller(){}

    DCC_Controller(Msgs_Pool * pool){
      this->pool = pool;
    }

    int getDecoderIndex(int address){
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

    void processFrame(String frame){
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

    void CmdSpeed(String frame){
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
    
    void CmdFunction(String frame){
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

    void CmdEmergencyStop(){

    }

    void CmdRelease(){

    }

    void CmdChangeCV(){
      // you must reset the arduino after using this command.
      // Otherwise, the repeated instruction frame will interfere other frames.
      Packet m = DCC_Packet_Generator::getConfigurationVariableAccessInstructionPacket(0x03,1,0x05);
      pool->fill(m);
    }
};

#endif

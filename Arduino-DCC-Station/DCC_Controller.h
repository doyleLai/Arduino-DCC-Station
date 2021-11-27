#ifndef DCC_Controller_h
#define DCC_Controller_h

#include "Packets_Pool.h"
#include "Decoder.cpp"
#include "Packet.h"

#define MAX_DECODERS 10

//void SetupTimer2();
class DCC_Controller{
  private:
    Packets_Pool * pool;
    Decoder decoders[MAX_DECODERS];
    uint8_t decoderCount ;

  public:
    DCC_Controller();
    //DCC_Controller(Msgs_Pool * pool);
    void DCC_begin();
    uint8_t getDecoderIndex(uint8_t address);
    bool processCommand(String frame);
    bool CmdSpeed(String frame);
    bool CmdFunction(String frame);
    bool CmdEmergencyStop();
    bool CmdRelease();
    bool CmdChangeCV();
    Packet getNextPacket();
};

extern DCC_Controller DCC;

#endif

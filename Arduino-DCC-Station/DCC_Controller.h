#ifndef DCC_Controller_h
#define DCC_Controller_h

#include "Msgs_Pool.h"
#include "Decoder.cpp"
#include "Packet.h"

#define MAX_DECODERS 10

//void SetupTimer2();
class DCC_Controller{
  private:
    Msgs_Pool * pool;
    Decoder decoders[MAX_DECODERS];
    int decoderCount ;

  public:
    DCC_Controller();
    DCC_Controller(Msgs_Pool * pool);
    void DCC_begin();
    int getDecoderIndex(int address);
    void processFrame(String frame);
    void CmdSpeed(String frame);
    void CmdFunction(String frame);
    void CmdEmergencyStop();
    void CmdRelease();
    void CmdChangeCV();
    Packet getNextMsg();
};

extern DCC_Controller DCC;

#endif

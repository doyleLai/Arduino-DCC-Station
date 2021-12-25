#ifndef DCC_Controller_h
#define DCC_Controller_h

#include <Arduino.h>
#include "Packets_Pool.h"
#include "Decoder.h"
#include "Packet.h"

#define MAX_DECODERS 10

// Defines which type of packet should be cached for output
typedef enum {
  Startup, // When power up, send reset packet
  SendPacket, // Send packets from the packets pool
} DCC_signal_state_t;


//void SetupTimer2();
class DCC_Controller{
  private:
    Packets_Pool * pool;
    Decoder decoders[MAX_DECODERS];
    uint8_t decoderCount ;
    DCC_signal_state_t signal_state;

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
    DCC_signal_state_t getSignalState();
    Packet getNextPacket();
};

extern DCC_Controller DCC;

#endif

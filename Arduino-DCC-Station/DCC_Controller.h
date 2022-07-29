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
  EmergencyStop, // Set speed step of all trains to 0 in this state
  CV // Chnage Configuration Variable (Not used)
} DCC_control_state_t;


//void SetupTimer2();
class DCC_Controller{
  private:
    Packets_Pool pool;
    Decoder decoders[MAX_DECODERS];
    uint8_t decoderCount;
    DCC_control_state_t control_state;
    Packet resetPkt;
    Packet stopPkt;
  public:
    DCC_Controller();
    //DCC_Controller(Msgs_Pool * pool);
    void DCC_begin();
    uint8_t getDecoderIndex(uint8_t address);
    bool processCommand(char msg[]);
    bool CmdSpeed(char msg[]);
    bool CmdFunction(char msg[]);
    bool CmdEmergencyStop(char msg[]);
    bool CmdRelease(char msg[]);
    bool CmdChangeCV(char msg[]);
    bool CmdReset(char msg[]);
    //DCC_control_state_t getControlState();
    Packet getNextPacket();
};

extern DCC_Controller DCC;

#endif

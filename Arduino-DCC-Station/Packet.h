#ifndef Packet_h
#define Packet_h

#include <stdint.h>

struct Packet {
  enum Type
  {
    AOI, // Advanced Operations Instruction - 128 Speed Step Control
    FG1I, // Function Group One Instruction - FL and F1-F4 Function Control
    FG2I_F5F8, // Function Group Two Instruction - F5-F8 Function Control
    FG2I_F9F12, // Function Group Two Instruction - F9-F12 Function Control
    FEI_F13F20, // Feature Expansion Instruction - F13-F20 Function Control
    FEI_F21F28, // Feature Expansion Instruction - F21-F28 Function Control
    RESET,
    IDLE
  };
  
  unsigned char data[6];
  uint8_t len;
  Type type;
} ;

#endif

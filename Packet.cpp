#pragma once

struct Packet {
  enum Type
  {
    AOI,
    FG1I,
    FG2I1,
    FG2I2,
    FEI,
    RESET,
    IDLE
  };
  
  unsigned char data[7];
  unsigned int len;
  Type type;
} ;

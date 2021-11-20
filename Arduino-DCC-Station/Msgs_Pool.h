#ifndef Msgs_Pool_h
#define Msgs_Pool_h

#include <Arduino.h>
#include "DCC_Packet_Generator.cpp"
#include "Packet.h"
#define BUFFER_SIZE 50

class Msgs_Pool{
  private:
    Packet buffer[BUFFER_SIZE];
    int fetchPointer;
  public:
    Msgs_Pool();
    void add(int pos, Packet msg);
    void fill(Packet pkt);
    Packet getNextMsg();
};

#endif

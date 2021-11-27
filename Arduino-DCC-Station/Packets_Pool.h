#ifndef Packets_Pool_h
#define Packets_Pool_h

#include "DCC_Packet_Generator.cpp"
#include "Packet.h"
#define POOL_SIZE 50

class Packets_Pool{
private:
  Packet pool[POOL_SIZE];
  uint8_t fetchPointer;
public:
  Packets_Pool();
  void add(uint8_t pos, Packet pkt);
  void fill(Packet pkt);
  Packet getNextPacket();
};

#endif

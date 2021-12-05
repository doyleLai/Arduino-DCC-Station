#ifndef Packets_Pool_h
#define Packets_Pool_h

#include "DCC_Packet_Generator.cpp"
#include "Packet.h"
#define PRIORITY_POOL_SIZE 20
#define REPETITIVE_POOL_SIZE 60

class Packets_Pool{
private:
  Packet priorityPool[REPETITIVE_POOL_SIZE];
  Packet repetitivePool[REPETITIVE_POOL_SIZE];
  uint8_t priorityAddPointer;
  uint8_t priorityFetchPointer;
  uint8_t fetchPointer;
public:
  Packets_Pool();
  void add(uint8_t decoderIndex, Packet pkt);
  void fill(Packet pkt);
  Packet getNextPacket();
};

#endif

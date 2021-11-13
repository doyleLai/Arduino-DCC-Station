#ifndef Msgs_Pool_cpp
#define Msgs_Pool_cpp

#include <Arduino.h>
#include "DCC_Packet_Generator.cpp"
#include "Packet.cpp"
#define BUFFER_SIZE 50

class Msgs_Pool {
  private:
    Packet buffer[BUFFER_SIZE];
    //int addPointer = 0;
    int fetchPointer = 0;
  public:

    Msgs_Pool(){
        // Fills the buffer with a reset and idle packets
        noInterrupts();
        buffer[0] = DCC_Packet_Generator::getDigitalDecoderResetPacket();
        for(int i = 0; i < BUFFER_SIZE; i++){
            buffer[i] = DCC_Packet_Generator::getDigitalDecoderIdlePacket();
        }
        interrupts();
    }

    void add(int pos, Packet msg){
        noInterrupts();
        buffer[pos] = msg;
        interrupts();
        //addPointer = addPointer % BUFFER_SIZE;
    }

    void fill(Packet pkt){
        noInterrupts();
        for(int i = 0; i < BUFFER_SIZE; i++){
            buffer[i] = pkt;
        }
        interrupts();
    }

    Packet getNextMsg(){
        Packet m = buffer[fetchPointer++];
        fetchPointer = fetchPointer % BUFFER_SIZE;
        return m;
    }
};

#endif

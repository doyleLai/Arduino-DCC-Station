#ifndef Msgs_Pool_cpp
#define Msgs_Pool_cpp

#include <Arduino.h>
#include "DCC_Packet_Generator.cpp"
#include "Packet.h"
#include "Msgs_Pool.h"

Msgs_Pool::Msgs_Pool(){
       fetchPointer = 0;
        // Fills the buffer with a reset and idle packets
        noInterrupts();
        buffer[0] = DCC_Packet_Generator::getDigitalDecoderResetPacket();
        for(int i = 0; i < BUFFER_SIZE; i++){
            buffer[i] = DCC_Packet_Generator::getDigitalDecoderIdlePacket();
        }
        interrupts();
}

void Msgs_Pool::add(int pos, Packet msg){
        noInterrupts();
        buffer[pos] = msg;
        interrupts();
        //addPointer = addPointer % BUFFER_SIZE;
}

void Msgs_Pool::fill(Packet pkt){
        noInterrupts();
        for(int i = 0; i < BUFFER_SIZE; i++){
            buffer[i] = pkt;
        }
        interrupts();
}

Packet Msgs_Pool::getNextMsg(){
        Packet m = buffer[fetchPointer++];
        fetchPointer = fetchPointer % BUFFER_SIZE;
        return m;
}

#endif

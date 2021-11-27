#include <avr/interrupt.h>
#include "DCC_Packet_Generator.cpp"
#include "Packet.h"
#include "Packets_Pool.h"

Packets_Pool::Packets_Pool(){
	fetchPointer = 0;
	cli(); // Disables interrupts
	//pool[0] = DCC_Packet_Generator::getDigitalDecoderResetPacket();
	// Fills the pool with idle packets
	Packet pkt = DCC_Packet_Generator::getDigitalDecoderIdlePacket();
	for (uint8_t i = 0; i < POOL_SIZE; i++){
		pool[i] = pkt;
	}
	sei(); // Enables interrupts
}

void Packets_Pool::add(uint8_t pos, Packet pkt){
	cli();
	pool[pos] = pkt;
	sei();
	//addPointer = addPointer % BUFFER_SIZE;
}

void Packets_Pool::fill(Packet pkt){
	cli();
	for (uint8_t i = 0; i < POOL_SIZE; i++){
		pool[i] = pkt;
	}
	sei();
}

Packet Packets_Pool::getNextPacket(){
	// This function is called from the ISR, no need to disable interrupt.
	Packet m = pool[fetchPointer++];
	fetchPointer = fetchPointer % POOL_SIZE;
	return m;
}

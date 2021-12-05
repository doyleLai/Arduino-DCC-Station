#include <avr/interrupt.h>
#include "DCC_Packet_Generator.cpp"
#include "Packet.h"
#include "Packets_Pool.h"

Packets_Pool::Packets_Pool(){
    priorityAddPointer = 0;
    priorityFetchPointer = 0;
	fetchPointer = 0;
	cli(); // Disables interrupts
	// Fills the pool with idle packets
	Packet pkt = DCC_Packet_Generator::getDigitalDecoderIdlePacket();
	for (uint8_t i = 0; i < REPETITIVE_POOL_SIZE; i++){
		repetitivePool[i] = pkt;
	}
	sei(); // Enables interrupts
}

void Packets_Pool::add(uint8_t decoderIndex, Packet pkt){
	// Only 6 types of pkts are stored in repetitivePool, pkt.type returns the type index.
	uint8_t pos = decoderIndex * 6 + pkt.type;
	cli();
	repetitivePool[pos] = pkt;

	priorityPool[priorityAddPointer] = pkt;
	priorityAddPointer = (priorityAddPointer + 1) % PRIORITY_POOL_SIZE;
	priorityPool[priorityAddPointer] = pkt;
	priorityAddPointer = (priorityAddPointer + 1) % PRIORITY_POOL_SIZE;
	sei();
	//addPointer = addPointer % BUFFER_SIZE;
}

void Packets_Pool::fill(Packet pkt){
	cli();
	for (uint8_t i = 0; i < REPETITIVE_POOL_SIZE; i++){
		repetitivePool[i] = pkt;
	}
	sei();
}

Packet Packets_Pool::getNextPacket(){
	// This function is called from the ISR, no need to disable interrupt.
	if (priorityAddPointer == priorityFetchPointer){
		// Nothing in priorityPool, fetchs from repetitivePool
		Packet m = repetitivePool[fetchPointer++];
		fetchPointer = fetchPointer % REPETITIVE_POOL_SIZE;
		return m;
	}
	else{
		// fetchs from priorityPool
		Packet m = priorityPool[priorityFetchPointer++];
		priorityFetchPointer = priorityFetchPointer % PRIORITY_POOL_SIZE;
		return m;
	}
}

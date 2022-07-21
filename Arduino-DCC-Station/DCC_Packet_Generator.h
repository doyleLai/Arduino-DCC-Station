#ifndef DCC_Packet_Generator_h
#define DCC_Packet_Generator_h

#include <stdint.h>
#include "Decoder.h"
#include "Packet.h"

// Check out the DCC packet format from this website.
// https://www.nmra.org/sites/default/files/s-9.2.1_2012_07.pdf

class DCC_Packet_Generator {
  public:
    static Packet getSpeedPacket(Decoder decoder);
    static Packet getFunctionGroup1Packet(Decoder decoder);
    static Packet getFunctionGroup2_1Packet(Decoder decoder);
    static Packet getFunctionGroup2_2Packet(Decoder decoder);
    static Packet getFeatureExpansionF13F20Packet(Decoder decoder);
    static Packet getFeatureExpansionF21F28Packet(Decoder decoder);
    static Packet getDigitalDecoderResetPacket();
    static Packet getDigitalDecoderIdlePacket();
    static Packet getDigitalDecoderBroadcastStopPacket();
    static Packet getConfigurationVariableAccessInstructionPacket(int address, unsigned int cv, int data);
};

#endif

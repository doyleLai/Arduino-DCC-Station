#pragma once

#include "Decoder.cpp"
#include "Packet.cpp"

// Check out the DCC packet format from this website.
// https://www.nmra.org/sites/default/files/s-9.2.1_2012_07.pdf

class DCC_Packet_Generator {
  public:
    static Packet getSpeedMessage(Decoder decoder) {
      // Advanced Operations Instruction (001)
      // 128 Speed Step Control
      //  { { locoAdr, 0x3F,  0, 0, 0, 0, 0}, 4},
      unsigned char data;

      Packet m;
      m.len = 4;
      m.data[0] = decoder.GetAddress();
      m.data[1] = 0x3F;

      int speedStep = decoder.speedStep;
      if (speedStep == 1)  {  // this would result in emergency stop
        speedStep = 0;
      }

      data = (decoder.dir) ? 0x80 : 0x00;
      data |=  speedStep;

      m.data[2] = data;

      // add XOR byte
      m.data[3] = (m.data[0] ^ m.data[1]) ^ data;

      m.type = Packet::Type::AOI;
      return m;
    }

    static Packet getFunctionGroup1Message(Decoder decoder) {
      // Function Group One Instruction (100)
      // F1 to F4 and FL
      Packet m;
      m.len = 3;
      m.data[0] = decoder.GetAddress();
      unsigned char data = 0x80;

      data |=  decoder.functions[0] << 0;
      data |=  decoder.functions[1] << 1;
      data |=  decoder.functions[2] << 2;
      data |=  decoder.functions[3] << 3;
      data |=  decoder.fL << 4;

      m.data[1] = data;
      m.data[2] = (m.data[0] ^ data) ;

      m.type = Packet::Type::FG1I;
      return m;
    }

    static Packet getFunctionGroup2Message1(Decoder decoder) {
      // Function Group Two Instruction (101)
      // F5 to F8
      // This instruction has the format 101SDDDD, S = 1
      Packet m;
      m.len = 3;
      m.data[0] = decoder.GetAddress();
      unsigned char data = 0xB0;

      data |=  decoder.functions[4] << 0;
      data |=  decoder.functions[5] << 1;
      data |=  decoder.functions[6] << 2;
      data |=  decoder.functions[7] << 3;

      m.data[1] = data;
      m.data[2] = (m.data[0] ^ data) ;

      m.type = Packet::Type::FG2I1;
      return m;
    }
    static Packet getFunctionGroup2Message2(Decoder decoder) {
      // Function Group Two Instruction (101)
      // F9 to F12
      // This instruction has the format 101SDDDD, S = 0
      Packet m;
      m.len = 3;
      m.data[0] = decoder.GetAddress();
      unsigned char data = 0xA0;

      data |=  decoder.functions[8] << 0;
      data |=  decoder.functions[9] << 1;
      data |=  decoder.functions[10] << 2;
      data |=  decoder.functions[11] << 3;

      m.data[1] = data;
      m.data[2] = (m.data[0] ^ data) ;

      m.type = Packet::Type::FG2I2;
      return m;
    }

    static Packet getFeatureExpansionF13F20Message(Decoder decoder) {
      // Feature Expansion Instruction (110)
      // F13 to F20
      // Two byte instructions: 110CCCCC 0 DDDDDDDD
      // CCCCC = 11110
      Packet m;
      m.len = 4;
      m.data[0] = decoder.GetAddress();
      m.data[1] = 0xDE;
      unsigned char data = 0x00;

      for (int i = 0; i < 8; i++) {
        data |=  decoder.functions[12 + i] << i;
      }
      
      m.data[2] = data;
      m.data[3] = (m.data[0] ^ m.data[1]) ^ data ;

      m.type = Packet::Type::FEI;
      return m;
    }

    static Packet getDigitalDecoderResetPacket() {
      // Digital Decoder Reset Packet For All Decoders
      // erase all volatile memory (including any speed and direction data)
      // bring the locomotive to an immediate stop
      // { { 0, 0, 0, 0, 0, 0, 0}, 3},
      Packet m;
      m.len = 3;
      m.data[0] = 0x00;
      m.data[1] = 0x00;      
      m.data[2] = 0x00;

      return m;
    }

    static Packet getDigitalDecoderIdlePacket() {
      // Digital Decoder Idle Packet For All Decoders
      // Digital Decoders shall perform no new action
      // { { 0xFF, 0, 0xFF, 0, 0, 0, 0}, 3},
      Packet m;
      m.len = 3;
      m.data[0] = 0xFF;
      m.data[1] = 0x00;      
      m.data[2] = 0xFF;

      return m;
    }


    
    static Packet getConfigurationVariableAccessInstructionPacket(int address, unsigned int cv, int data) {
      // Configuration Variable Access Instruction (111) 
      // Configuration Variable Access Instruction - Long Form 
      // 1110CCVV 0 VVVVVVVV 0 DDDDDDDD 
      // Write byte only (CC=11) 

      cv -= 1;
      unsigned char highCvAddress =  cv >> 8 & 0x03;
      unsigned char LowCvAddress =  0xFF & cv ;

      Packet m;
      m.len = 5;
      m.data[0] = address;
      m.data[1] =  0xEC | highCvAddress;    
      m.data[2] = LowCvAddress;
      m.data[3] = data;
      m.data[4] = m.data[0] ^ m.data[1] ^ m.data[2] ^ m.data[3];
      return m;
    }
};

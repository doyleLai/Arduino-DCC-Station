#include "DCC_Packet_Generator.h"
#include <stdint.h>
#include "Decoder.h"
#include "Packet.h"

// Check out the DCC packet format from this website.
// https://www.nmra.org/sites/default/files/s-9.2.1_2012_07.pdf


Packet DCC_Packet_Generator::getSpeedPacket(Decoder decoder) {
      // Advanced Operations Instruction (001)
      // 128 Speed Step Control
      // This instruction has the format 001CCCCC 0 DDDDDDDD
      // CCCCC = 11111 for 128 Speed Step Control
      // D:7 = 1 for forward, 0 for reverse
      // D:0-6 = 0000000 for stop, 0000001 for emergency stop.
      unsigned char data;

      Packet m;
      m.len = 4; // 1 address byte, 2 instruction bytes and 1 error control byte
      m.data[0] = decoder.GetAddress();
      m.data[1] = 0x3F;

      uint8_t speedStep = decoder.speedStep;

      data = (decoder.dir) ? 0x80 : 0x00;
      data |=  speedStep;

      m.data[2] = data;

      // add XOR byte
      m.data[3] = (m.data[0] ^ m.data[1]) ^ data;

      m.type = Packet::Type::AOI;
      return m;
    };

Packet DCC_Packet_Generator::getFunctionGroup1Packet(Decoder decoder) {
      // Function Group One Instruction (100)
      // F1 to F4 and FL
      Packet m;
      m.len = 3;
      m.data[0] = decoder.GetAddress();
      unsigned char data = 0x80;

      data |=  decoder.functions[1] << 0; // F1
      data |=  decoder.functions[2] << 1; // F2
      data |=  decoder.functions[3] << 2; // F3
      data |=  decoder.functions[4] << 3; // F4
      data |=  decoder.functions[0] << 4; // FL

      m.data[1] = data;
      m.data[2] = (m.data[0] ^ data) ;

      m.type = Packet::Type::FG1I;
      return m;
    }

Packet DCC_Packet_Generator::getFunctionGroup2_1Packet(Decoder decoder) {
      // Function Group Two Instruction (101)
      // F5 to F8
      // This instruction has the format 101SDDDD, S = 1
      Packet m;
      m.len = 3;
      m.data[0] = decoder.GetAddress();
      unsigned char data = 0xB0;

      data |=  decoder.functions[5] << 0;
      data |=  decoder.functions[6] << 1;
      data |=  decoder.functions[7] << 2;
      data |=  decoder.functions[8] << 3;

      m.data[1] = data;
      m.data[2] = (m.data[0] ^ data) ;

      m.type = Packet::Type::FG2I_F5F8;
      return m;
    }
Packet DCC_Packet_Generator::getFunctionGroup2_2Packet(Decoder decoder) {
      // Function Group Two Instruction (101)
      // F9 to F12
      // This instruction has the format 101SDDDD, S = 0
      Packet m;
      m.len = 3;
      m.data[0] = decoder.GetAddress();
      unsigned char data = 0xA0;

      data |=  decoder.functions[9] << 0;
      data |=  decoder.functions[10] << 1;
      data |=  decoder.functions[11] << 2;
      data |=  decoder.functions[12] << 3;

      m.data[1] = data;
      m.data[2] = (m.data[0] ^ data) ;

      m.type = Packet::Type::FG2I_F9F12;
      return m;
    }

Packet DCC_Packet_Generator::getFeatureExpansionF13F20Packet(Decoder decoder) {
      // Feature Expansion Instruction (110)
      // F13 to F20
      // Two byte instructions: 110CCCCC 0 DDDDDDDD
      // CCCCC = 11110
      Packet m;
      m.len = 4;
      m.data[0] = decoder.GetAddress();
      m.data[1] = 0xDE;
      unsigned char data = 0x00;

      for (uint8_t i = 0; i < 8; i++) {
        data |=  decoder.functions[13 + i] << i;
      }
      
      m.data[2] = data;
      m.data[3] = (m.data[0] ^ m.data[1]) ^ data ;

      m.type = Packet::Type::FEI_F13F20;
      return m;
    }

    // Not yet tested
Packet DCC_Packet_Generator::getFeatureExpansionF21F28Packet(Decoder decoder) {
      // Feature Expansion Instruction (110)
      // F21 to F28
      // Two byte instructions: 110CCCCC 0 DDDDDDDD
      // CCCCC = 11111
      Packet m;
      m.len = 4;
      m.data[0] = decoder.GetAddress();
      m.data[1] = 0xDF;
      unsigned char data = 0x00;

      for (uint8_t i = 0; i < 8; i++) {
        data |=  decoder.functions[21 + i] << i;
      }
      
      m.data[2] = data;
      m.data[3] = (m.data[0] ^ m.data[1]) ^ data ;

      m.type = Packet::Type::FEI_F21F28;
      return m;
    }

Packet DCC_Packet_Generator::getDigitalDecoderResetPacket() {
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

Packet DCC_Packet_Generator::getDigitalDecoderIdlePacket() {
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


    
Packet DCC_Packet_Generator::getConfigurationVariableAccessInstructionPacket(int address, unsigned int cv, int data) {
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


#include "Decoder.h"
#include <stdint.h>
// F0 = FL (Lighting)


    Decoder::Decoder() {
    }

    Decoder::Decoder(uint8_t addr) {
      this->address = addr;
    }

    void Decoder::SetFunc(uint8_t func) {
      this->functions[func] = true;
    }
    
    void Decoder::ClearFunc(uint8_t func) {
      this->functions[func] = false;
    }

    bool Decoder::ToggleFunc(uint8_t func) {
      this->functions[func ] ^= true;
      return this->functions[func];
    }

    uint8_t Decoder::GetAddress() {
      return this->address;
    }



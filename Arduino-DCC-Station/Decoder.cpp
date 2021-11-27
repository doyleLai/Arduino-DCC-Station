#ifndef Decoder_cpp
#define Decoder_cpp
#include <stdint.h>
// F0 = FL (Lighting)

class Decoder {
  private:
    uint8_t address = 0;
  public:
    uint8_t speedStep = 0;
    bool dir = 1;
    bool functions[30] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    Decoder() {
    }

    Decoder(uint8_t addr) {
      this->address = addr;
    }

    void SetFunc(uint8_t func) {
      this->functions[func] = true;
    }
    
    void ClearFunc(uint8_t func) {
      this->functions[func] = false;
    }

    bool ToggleFunc(uint8_t func) {
      this->functions[func ] ^= true;
      return this->functions[func];
    }

    uint8_t GetAddress() {
      return this->address;
    }
};


#endif

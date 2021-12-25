#ifndef Decoder_h
#define Decoder_h
#include <stdint.h>
// F0 = FL (Lighting)

class Decoder {
  private:
    uint8_t address = 0;
  public:
    uint8_t speedStep = 0;
    bool dir = 1;
    bool functions[30] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    Decoder();
    Decoder(uint8_t addr);
    void SetFunc(uint8_t func);
    void ClearFunc(uint8_t func);
    bool ToggleFunc(uint8_t func);
    uint8_t GetAddress();
};

#endif

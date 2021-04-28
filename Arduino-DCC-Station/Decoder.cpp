#pragma once
#include <Arduino.h>
// F0 = FL (Lighting)

class Decoder {
  private:
    int address = 0;
  public:
    int speedStep = 0;
    bool dir = 1;
    bool functions[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    bool fL = false;

    Decoder() {
    }

    Decoder(int addr) {
      this->address = addr;
    }

    void SetFunc(int func) {
      if (func == 0) {
        this->fL = true;
      }
      else {
        this->functions[func - 1] = true;
      }
    }
    void ClearFunc(int func) {
      if (func == 0) {
        this->fL = false;
      }
      else {
        this->functions[func - 1] = false;
      }
    }
    bool ToggleFunc(int func) {
      if (func == 0) {
        this->fL ^= true;
        return this->fL;
      }
      else {
        this->functions[func - 1] ^= true;
        return this->functions[func - 1];
      }
    }

    int GetAddress() {
      return this->address;
    }

    String toString(){
      String s = "Address:" + String(address) + ", Speed: " + String(speedStep)
      +", Funcs: "+  String(functions[0])+String(functions[1])+String(functions[2])+String(functions[3])+String(functions[4]);
      return s;
    }
};

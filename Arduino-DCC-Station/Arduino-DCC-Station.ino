// This project is available on https://github.com/doyleLai/Arduino-DCC-Station

#include "DCC_Controller.h"
#define INPUT_LEN_MAX 10
#define EN_PIN 10

bool isStarted = false;
char inString[INPUT_LEN_MAX];
uint8_t inPos = 0;
bool result;

void setup(void) {
  pinMode(EN_PIN, OUTPUT);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  DCC.DCC_begin();
  digitalWrite(EN_PIN, HIGH);
}

void loop(void) {
  while (Serial.available()) {
    char inChar = Serial.read();
    if (inChar == '<'){
      isStarted = true;
      inPos = 0;
    }
    else if (inChar == '>' && isStarted){
      inString[inPos] = '\0';
      result = DCC.processCommand(inString);
      Serial.print(inString);
      Serial.println(result? " ok" : " error");
      isStarted = false;
      inString[0] = '\0';
    }
    else{
      inString[inPos++] = inChar;
      inPos %= INPUT_LEN_MAX;
    }
  }
}

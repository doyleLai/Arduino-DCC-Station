// This project is available on https://github.com/doyleLai/Arduino-DCC-Station

#include "DCC_Controller.h"
#define INPUT_LEN_MAX 10
bool isStarted = false;
//char inChar;
char inString[INPUT_LEN_MAX];
uint8_t inPos = 0;
//String inString = "";    // string to hold input
bool result;

void setup(void) {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  DCC.DCC_begin();
}

void loop(void) {
  //delay(1);
  while (Serial.available()) {
    char inChar = Serial.read();
    //Serial.println("R");
    if (inChar == '<'){
      isStarted = true;
      inPos = 0;
      //inString[inPos++] = inChar;
    }
    else if (inChar == '>' && isStarted){
      inString[inPos] = '\0';
      Serial.println(inString);
      result = DCC.processCommand(inString);
      //Serial.println(result? "ok":"error");
      isStarted = false;
      inString[0] = '\0';
    }
    else{
      inString[inPos++] = inChar;
      inPos %= INPUT_LEN_MAX;
    }
  }
}

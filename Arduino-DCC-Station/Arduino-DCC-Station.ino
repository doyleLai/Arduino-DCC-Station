// This project is available on https://github.com/doyleLai/Arduino-DCC-Station

#include "DCC_Controller.h"

bool isStarted = false;
int inChar;
String inString = "";    // string to hold input

void setup(void) {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  DCC.DCC_begin();
}

void loop(void) {
  //delay(1);
  while (Serial.available()) {
    inChar  = Serial.read();
    //Serial.println("R");
    if(inChar == '<'){
      isStarted = true;
      inString = "";
    }
    else if (inChar == '>'){
      Serial.println(inString);
      DCC.processFrame(inString);
      isStarted = false;
    }
    else if (isStarted){
      inString += (char)inChar;
    }
  }
}

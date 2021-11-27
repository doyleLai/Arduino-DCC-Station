// This project is available on https://github.com/doyleLai/Arduino-DCC-Station

#include "DCC_Controller.h"

bool isStarted = false;
char inChar;
String inString = "";    // string to hold input
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
    inChar  = Serial.read();
    //Serial.println("R");
    if(inChar == '<'){
      isStarted = true;
      inString = "";
    }
    else if (inChar == '>'){
      Serial.print(inString + " ");
      result = DCC.processCommand(inString);
      Serial.println(result? "ok":"error");
      isStarted = false;
    }
    else if (isStarted){
      inString += (char)inChar;
    }
  }
}

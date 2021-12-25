// The way to genreate DCC signal is based on the program code from Michael Blank
// which was written on 23 November 2009

#include "DCC_Signal.h"
#include <Arduino.h>
#include "DCC_Controller.h"
#include "Packet.h"
#include "DCC_Packet_Generator.h"

#if defined(__AVR_ATmega328P__)

//Timer frequency is 2MHz for ( /8 prescale from 16MHz )
#define TIMER_SHORT 116  // 58usec pulse length
#define TIMER_LONG 200  // 100usec pulse length

static inline void Drive0();
static inline void Drive1();
static void handle_interrupt(volatile uint8_t & TCNTx, volatile uint8_t & OCRx);

// DCC Output Pins: pin 11 (PB.3) and 12 (PB.4)
void SetupPins(){
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
}

// Uses Timer2.
 void SetupTimer() {
  // Normal Mode, OC2A OC2B disconnected. clk / 8 prescaler
  TCCR2A = 0x00; 
  TCCR2B = 0x00 | (1 << CS21); 
  TIMSK2 = 0x00 | (1 << OCIE2A); // Compare Match A interrupt enabled

  OCR2A = TIMER_SHORT;
  TCNT2 = 0;
}

static inline void Drive0(){
  PORTB |= (1 << 3);
  PORTB &= ~(1 << 4);
}

static inline void Drive1(){
  PORTB |= (1 << 4);
  PORTB &= ~(1 << 3);
}

ISR(TIMER2_COMPA_vect) {
  handle_interrupt(TCNT2, OCR2A);
}

#else
#error "Unsupported hardware"
#endif

static void handle_interrupt(volatile uint8_t & TCNTx, volatile uint8_t & OCRx){
  static DCC_pulse_state_t current_state = Preamble;
  static uint8_t timerValue = TIMER_SHORT;  // store last timer value
  //static unsigned char flag = 0;              // used for short or long pulse
  static bool isSecondPulse = false;
  static uint8_t preamble_count = 16;
  static unsigned char outbyte = 0;
  static unsigned char cbit = 0x80;
  static uint8_t byteIndex = 0;
  static Packet cachedMsg = DCC_Packet_Generator::getDigitalDecoderIdlePacket();
  static Packet resetPkt = DCC_Packet_Generator::getDigitalDecoderResetPacket();

  // for every second interupt just toggle signal
  if (isSecondPulse) {
    Drive1();
    isSecondPulse = false;
    // set timer to last value
    OCRx = OCRx + timerValue;

  } else {  // != every second interrupt, advance bit or state
    Drive0();
    isSecondPulse = true;

    switch (current_state) {
      case Preamble:
        timerValue = TIMER_SHORT; // This pulse is short 
        preamble_count--;
        if (preamble_count == 0) {  // advance to next state
          // preamble completed, get next message
          current_state = Seperator;
          byteIndex = 0;  //start msg with byte 0
          if (DCC.getSignalState()== SendPacket){
            cachedMsg = DCC.getNextPacket();
          }
          else{
            cachedMsg = resetPkt;
          }
        }
        break;
      case Seperator:
        timerValue = TIMER_LONG; // This pulse is long 
        // then advance to next state
        current_state = SendByte;
        // goto next byte ...
        cbit = 0x80;  // send this bit next time first (0x80 = 1000 0000)
        outbyte = cachedMsg.data[byteIndex];
        break;
      case SendByte:
        if (outbyte & cbit) {
          timerValue = TIMER_SHORT; // send short pulse
        } else {
          timerValue = TIMER_LONG; // send long pulse
        }
        cbit = cbit >> 1;
        if (cbit == 0) {  // last bit sent, is there a next byte?
          byteIndex++;
          if (byteIndex >= cachedMsg.len) {
            // this was already the XOR byte then advance to preamble
            current_state = Preamble;
            preamble_count = 16;
          } else {
            // send separtor and advance to next byte
            current_state = Seperator;
          }
        }
        break;
    }
    OCRx = OCRx + timerValue;
  }
}

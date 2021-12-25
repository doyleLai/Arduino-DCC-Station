#ifndef DCC_SIGNAL_h
#define DCC_SIGNAL_h

#include <Arduino.h>

void SetupPins();
void SetupTimer();

// Defines long pulse or short pulse should be sent.
typedef enum {
  Preamble, // short pulse
  Seperator, // long pulse
  SendByte // depends on the out bit.
} DCC_pulse_state_t;

#endif

# Arduino-DCC-Station
This project is adapted from Michael Blank's work. The original program supports one locomotive only. This program aims to support multiple locomotives.

## Hardware setup
You need a full H-bridge motor driver to boost the signal from the Arduino output. Connect pin 11 and 12 to the input pins of your H-bridge. Connect the output pins of the H-bridge to the rails.

If you use earth grounded oscilloscope to inspect the output signal, attach the probe tip only to the Arduino or the motor driver's output. Never connect the ground lead of the earth grounded oscilloscope to any signal pin.

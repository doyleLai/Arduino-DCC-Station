# Arduino-DCC-Station
This project is adapted from Michael Blank's work. The original program supports one locomotive only. This program aims to support multiple locomotives.

## Hardware Configuration
You need an H-bridge motor driver to deliver a large current flow used by locomotives. Connect Ardunio's pin 11 and 12 to the two input pins of your H-bridge. Connect the two output pins of the H-bridge to the rails.

If you use a non-isolated oscilloscope to inspect the Arduino or the motor driver's output, attach the probe tip only. Never connect the ground lead of a non-isolated oscilloscope to any signal pin.

## Command Messages
To be updated...

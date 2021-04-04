# Arduino-DCC-Station
This project is adapted from Michael Blank's work. The original program supports one locomotive only. This program aims to support multiple locomotives.

## Hardware Configuration
You need an H-bridge motor driver to deliver a large current flow used by locomotives. Connect Ardunio's pin 11 and 12 to the two input pins of your H-bridge. Connect the two output pins of the H-bridge to the rails.

If you use a non-isolated oscilloscope to inspect the Arduino or the motor driver's output, attach the probe tip only. Never connect the ground lead of a non-isolated oscilloscope to any signal pin.

## Control Messages
The program continuously reads control messages from the USB serial port and generates corresponding DCC messages. The states of locomotives, including the direction, speed step and function ON/OFFs of F0-F20 (F0 is the headlight control), are stored in the SRAM so the system can re-generate DCC messages for all locomotives periodically without the need of repeating control messages.

### Message formet
Each message is enclosed by "<>" and has the following format:
```
Type (1 byte), Address (1 byte), Payload
```
#### Type S (speed control)
Payload is 1 byte of direction [0 (backward),1 (forward)], followed by 3-byte fixed size of 128 speed steps value [%d3].
#### Type F (Function control)
Payload is a 2-byte fixed size of the index of function F00-F20 [%d2], followed by 1 byte of on/off [1,0].
F0 is the control of the headlight.

Example:
```
<S31005>
```
This message is to set the speed of address 3 decoder to forward direction and speed steps 5.

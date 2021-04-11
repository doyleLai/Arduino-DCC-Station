# Arduino-DCC-Station
This project is adapted from Michael Blank's work. The original program supports one locomotive only. This program aims to support multiple locomotives.

## Hardware Configuration
You need an H-bridge motor driver to deliver a large current flow used by locomotives. Connect Ardunio's pin 11 and 12 to the two input pins of your H-bridge. Connect the two output pins of the H-bridge to the rails.

If you use a non-isolated oscilloscope to measure the Arduino or the motor driver's output pulses, attach the probe tip only. Never connect the ground lead of a non-isolated oscilloscope to any signal pin.

## Control Messages
The program continuously reads control messages from the USB serial port and generates corresponding DCC signals. The states of locomotives, including the direction, speed step and function ON/OFFs of F0-F20 (F0 refers to headlight), are stored in the SRAM so the system can re-generate DCC messages for all locomotives periodically without the need of repeating control messages.

### Message format
Each message is enclosed by "<>" and has the following format:
```
Type (1 byte), Address (1 byte), Payload
```
#### Type S - Speed control
```
<SADVVV>
```
This type of message controls the direction and speed step of a locomotive.
The payload is 1 byte of direction D [0 (backward),1 (forward)], followed by 3-byte fixed size value of 128 speed step VVV [000-128].

#### Type F - Function control
```
<FAUUE>
```
The payload is a 2-byte fixed size value of the index of function F0-F20 UU [00-20], followed by 1 byte value of on/off E [1,0].
* F0 is the control of the headlight.

#### Example 1:
The following message sets the speed of address 3 decoder to forward direction and speed steps 5.
```
<S31005>
```
* Type (T): S
* Address (A): 3
* Direction (D): 1
* Speed Step (VVV): 005

#### Example 2:
The following message turns ON the F2 of address 3 decoder.
```
<F3021>
```
* Type (T): F
* Address (A): 3
* Function (UU): 02
* ON/OFF (O): 1





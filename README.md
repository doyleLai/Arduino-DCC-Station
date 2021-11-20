# Arduino-DCC-Station
This project was adapted from Michael Blank's work. The original program supports one decoder only. This program aims to support up to 9 decoders.

## Hardware Configuration
You need an H-bridge motor driver to deliver the large current flow used by locomotives. Do not connect Arduino pins to the track and locomotives directly. To set up the circuit, connect the pin 11 and 12 on Arduino to the two input pins on your H-bridge. Connect the two output pins on the H-bridge to the track.

This program was tested on Arduino UNO Rev3 and should work on other ATmega328p boards.

>If you use a non-isolated oscilloscope to measure the output pulses on Arduino or the motor driver, attach the probe tip only. Never connect the ground lead of a non-isolated oscilloscope to any signal pin.

## How to use it
1. Download and unzip the project.
2. Open the folder "Arduino-DCC-Station" with Arduino IDE.
3. Upload the program to your Arduino board.
4. Wire the Arduino board, moter driver and the tracks. Make sure you have used an external power source to power up the motor driver and the tracks.
5. Open Serial Monitor and send control messages (see below) to control your locos. 

## Control Messages
The program continuously reads control messages from the USB serial port and generates corresponding DCC signals. The states of locomotives, including the direction, speed step and function ON/OFFs of F0-F20 (F0 refers to headlight), are stored in the SRAM so the system can re-generate DCC messages for all locomotives periodically without the need of repeating sending control messages.

### Message format
Each message is enclosed by "<>" and has the following format:
```
Type (1 byte), Address (1 byte), Payload
```
### Type S - Speed control
```
<SADVVV>
```
This type of message controls the direction and speed step of a locomotive.
The payload consists of 1 byte of direction D [0 (backward),1 (forward)], followed by 3 bytes of 128 speed step VVV [000-127].

#### Example
Sets the speed of address 3 decoder to forward direction and speed step 5.
```
<S31005>
```
* Type (T): S
* Address (A): 3
* Payload
  * Direction (D): 1
  * Speed Step (VVV): 005

### Type F - Function control
```
<FAUUE>
```
The payload consists of 2 bytes of function index F0-F20 UU [00-20], followed by 1 byte of on/off E [1,0].
* F0 is the function control of the headlight.

#### Example
Turns ON the F2 of address 3 decoder.
```
<F3021>
```
* Type (T): F
* Address (A): 3
* Payload
  * Function (UU): 02
  * ON/OFF (E): 1

## Disclaimer
You should take your own risk to use anything in this project. The author of the project is not responsible for any damage, hurt, or any kind of accident caused by the use of the program code and the suggested electronic components.

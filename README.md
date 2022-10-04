# Arduino-DCC-Station
A simple Arduino model train control (DCC) system acts as a command station to control DCC decoders.

This project adapted Michael Blank's [program code](http://www.oscale.net/en/simpledcc) for signal generation. The original program supports one decoder only. This program supports up to 9 decoders and provides command controls interface on the serial port.

The program generates two opposite DCC signals. When one signal goes HIGH, the other goes LOW, and vice versa. That means you can connect them to an H-bridge circuit to boost up the signal to power the track and locomotives.

## Board Compatibility
The program uses the hardware timer and interrupts to achieve the precise pulse width generations of the DCC signal. The following Arduino boards are supported.

- Arduino UNO (Other ATmega328p based boards may be supported)
- Arduino UNO WiFi Rev.2 (Other ATmega4809 based boards may be supported)

## Wiring
Connect pin 11 and pin 12 on Arduino to the two input pins of the H-bridge. Connect the two output pins of the H-bridge to the track. Be sure you use an H-bridge to deliver the large current flow used by locomotives. Do not connect Arduino pins to the track nor locomotives directly. Also, make sure you use an external power source instead of the USB 5v to power the H-bridge. The nominal voltages limit for N and HO scale are 12v and 15v according to the NMRA standard ([S-9.1](https://www.nmra.org/sites/default/files/standards/sandrp/pdf/s-9.1_electrical_standards_for_digital_command_control_2021.pdf)).

## Installation
1. Download and unzip the project.
2. Open the folder "Arduino-DCC-Station" with Arduino IDE.
3. Upload the sketch to your Arduino board.
4. Connect the Arduino to the H-bridge circuit and the track.
5. Open Serial Monitor and send control messages (see below) to control your locos. 

## Control Messages
The program reads control messages from the default serial port and generates corresponding DCC signals. The states of locomotives, including the direction, speed step and function ON/OFFs of F0-F20 (F0 refers to headlight), are stored in the SRAM so the system will output DCC packets to the rails continuously without having the need of resending control messages.

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
* Type: S
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
* Type: F
* Address (A): 3
* Payload
  * Function (UU): 02
  * ON/OFF (E): 1
  
### Type E - Emergency stop
```
<E>
```
Stop all locomotives. This message will instruct all digital decoders shall stop delivering energy to the motor.

### Type R - Release emergency stop
```
<R>
```
Resume normal operation. Locomotives will remain stationary unless a new speed step is set. 

## Repetition of packets Transmission
Some packets are sent repeatedly as per the requirement of NMRA standard ([S-9.2](https://www.nmra.org/sites/default/files/s-92-2004-07.pdf)) and to keep the consistency of the controller and locos’ behaviours. The NMRA standard only mentioned packets should be repeated as frequently as possible but did not define how a particular packet should be repeated. This program has the following approach.

The program consists of two packet pools, which can be considered as output buffers. The priority pool, implemented as a queue (FIFO), stores packets to be sent first. The repetitive pool, implemented as a circulated array, stores packets that need repetition of output. Each slot in the repetitive pool is dedicated to a specific type of packet of a loco address. Six types of packets are needed for one loco. Hence, each loco occupies six slots in the repetitive pool. Packets stay there, and one is replaced by a new one when the associated state of the loco changes.

|Type of Packet | Function|
| ------------- | ------------- |
|Advanced Operations Instruction | 128 Speed Step Control|
|Function Group One Instruction | FL and F1-F4 Function Control|
|Function Group Two Instruction (F5-F8) | F5-F8 Function Control|
|Function Group Two Instruction (F9-F12) | F9-F12 Function Control|
|Feature Expansion Instruction (F13-F20) | F13-F20 Function Control|
|Feature Expansion Instruction (F21-F28) | F21-F28 Function Control|

When a control message comes, two identical packets of the corresponding command are pushed to the priority pool and one is written to the specific slot in the repetitive pool. The second packet to the priority pool compensates the first in case the first is lost during transmission. When a packet is available in the priority pool, the DCC signal generator will fetch the packet from there for the next output. This ensures that locos can act instantly to the control message. Once a packet is fetched from the priority pool, it is erased.

When the priority pool is empty, the generator fetches a packet from the respective pool in turn. Since packets in the repetitive pool stay there during the system lifetime, the system can continuously output meaningful packets even when no control message comes and keep locos' behaviours consistent with the controller.

## Disclaimer
You should take your own risk to use anything in this project. The author of the project is not responsible for any damage, hurt, or other kinds of accident caused by the use of the program code and the suggested electronic components.

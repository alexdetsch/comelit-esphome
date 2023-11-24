# comelit-esphome
Comelit Simplebus2 Interface for Home Assistant

**New**: *There is also a side-project that shares the same hardware: [comelit-esp8266](https://github.com/mansellrace/comelit-esp8266). it is a simplified and unrelated version of the home assistant world, created to allow decoding and interfacing the same protocol, with the same hardware, to interface the comelit bus even different home automation systems, to build intercom call repeaters, etc.*

## Introduction to the project

Initially, I wanted to modify my Comelit mini hands-free intercom 6721W to interface it with Home Assistant, so that I can receive a notification when someone intercoms me, and to be able to open the two doors controlled by the intercom conveniently remotely.

![Comelit mini](/images/comelit_mini.jpg)

The intercom works on Comelit's proprietary Simplebus2 2-wire bus.
I wanted to connect directly to the printed circuit board of the indoor station, which, however, uses the same speaker for ring and voice call, has touch buttons, the situation was getting complicated.
I discarded the idea of using a Ring Intercom, because although it works great and supports the Simplebus2 protocol, it does not allow you to control the opening of the second door, it is bulky, works only in the cloud, and has the problem of battery power.

I then discovered the wonderful work of **[plusvic](https://github.com/plusvic/simplebus2-intercom)** who analyzed and decoded the simplebus protocol, and made a ring repeater based on a PIC used to decode the protocol, wireless transmission chip, ESP8266.
**[aoihaugen](https://github.com/aoihaugen/simplebus2-intercom)** created a fork, and adapted the code for decoding the signal on arduino.
I want to give a huge thanks to both of you, without your work I would have never gotten to my goal, I took abundant cues from both of you for hardware and software.

In my implementation I used a Wemos d1 mini with Esphome-based firmware for easy integration on Home Assistant.

## Hardware

I adapted the circuit proposed by **[plusvic](https://github.com/plusvic/simplebus2-intercom)** by modifying it to fit the situation of my condominium system and the needs of esphome. The no-load voltage I detect on the bus wires is about 35V, and the data signal is superimposed on the DC voltage.
The exchange of information (commands) between indoor and outdoor station is through a 25kHz modulated data signal, in my case of peak-to-peak amplitude varying from 0.5V to 5V depending on the distance of the device that is transmitting. When audio/video communication is initiated, an additional high-frequency signal is added.

![Schema elettrico](/images/schematic.png)

### Power section

The circuit has the option of being powered directly from the bus.

I decided to use a DD4012SA step-down switching module that supports up to 40v input and gives 5v output.

The whole circuit draws about 20mA from the bus line, which increases to 160mA during data transmission.

I placed a 250mA fuse at the input to protect the circuit, followed by a rectifier bridge.

Power for the switching module is taken downstream from an RC low-pass filter, formed by R1 and C2, which eliminates fluctuations due to data transmission. After several tests, the best compromise between received and induced noise reduction on the bus and power dissipation was obtained with a 220Ω resistor, which dissipates just 75mW continuously.

### Data receiving section.

As in the circuit I was inspired by, the data signal is picked up from the bus by a high-pass type filter, in the schematic consisting of R5 and C1, 10kΩ and 1nF, respectively.

The filter has a cutoff frequency of about 15kHz, sufficient to reduce interference due to baseband audio communication.

The signal at the ends of the resistor is then given as input to a dual LM2903 comparator with an open collector type output. The threshold against which the signal is compared is set at approximately 0.25v via a resistive divider.

To simplify signal reception by the Wemos, I decided to insert a monostable stage, eliminating the 25-kHz carrier and taking advantage of the second comparator already on board the LM2903.

In the presence of signal, the output of the first comparator is brought low by discharging the 10nF capacitor C3; in the absence of signal, the capacitor is charged through the 220kΩ resistor R9.

The voltage at the ends of the capacitor is compared with a second fixed voltage, obtained from the same divider used on the first stage. The output of the second comparator is sent to the Wemos, which will then have data packets stripped of carrier oscillation as input.

In the presence of any input signal, the output of the second comparator remains high for an additional 1.5ms beyond the time the signal on the bus remains high. A time of 20µs would have sufficed to demodulate the command carrier, but increasing the timing this much allowed me to reduce the pickup of additional signals present on the bus, which disrupt the reception and decoding of commands.

### Data transmission section

Data transmission is accomplished by creating a pulsed absorption on the bus, modulated at 25kHz, with the timing dictated by the communication protocol. A digital pin on the wemos drives an NPN transistor, which must support voltages of at least 40v and currents of at least 150mA. In the prototype I made, I used a BC337.

The current flowing through the transistor is set by the value of the resistor placed on the collector. I chose to connect two 470Ω resistors in parallel to divide the power dissipation.

In my case, with higher resistor values, the commands sent by the wemos are not always received correctly. Depending on the distance to the bus power supply, it may be necessary to change the value.

### Hardware Implementation

Initially I made the circuit on breadboard, once the final schematic was finalized I made a pcb using SMD technology components, just 46 x 29 mm in size, which is slightly larger than the wemos D1 mini. In my case it fit perfectly inside the round box set up behind the intercom.

![pcb_3d](/images/pcb_3d.png) ![pcb](/images/pcb.png)

If the intercom system is powered from the condominium meter, you might decide not to power the wemos from the bus, not mounting the switching module and powering the wemos at 5v independently. In this way, the consumption of the wemos (0.5w) will not burden the condominium power supply

## Protocol

As already mentioned, the exchange of information between the various devices takes place via a protocol based on a 25Khz carrier. Such a carrier is always sent for 3ms. The duration of "silence" between two transmissions determines the value of the transmitted bit, 3ms corresponds to "0," 6ms corresponds to "1."

The transmission always begins with a 3ms pulse, followed by a 16ms silence.

Then 18 bits are sent, divided as follows:

- the first 8 bits indicate the address. The least significant bit is transmitted first.
- the next 6 bits indicate the command, again starting with the least significant bit.
- the next 4 bits are checksum, they simply indicate the number of bits to 1 sent in the previous fields.

For example, when I call an indoor intercom from the door station, the command 50 is sent. Suppose we call intercom #21:
- Command 50 = 110010
- Address 21 = 00010101
- Checksum 6 = 0110

The bit sequence sent is then 010011 10101000 0110

Commands thus formatted are then sent on the bus line to all apartment stations.

A command is also sent when a request is made from the indoor station to activate video communication when there is no call (command 20), when the main door is commanded to open (command 16), when the secondary door is commanded to open (command 29), when an indoor station receives the "out of door" call, when audio conversation is initiated or interrupted, when the call goes into timeout, etc.

## Software

As a software platform I used ESPhome for fast integration on home assistant.
I decided to take advantage of the *"remote receiver "* and *"remote transmitter "* components for decoding received bits.

Remote receiver automatically decodes the most common protocols related to remote controls or radio transmitters.

If native decoding is not available, raw mode can be used: each time a train of pulses is received, an automation is performed to which an array of integers is passed. The first number in the array, positive, indicates how many microseconds the input signal was at logic high value. The second value, negative, indicates how many microseconds the input signal was at logic low. Subsequent values follow the same logic, always alternating positive and negative numbers.

It goes without saying that just checking the value of the negative numbers in the array is enough to decode the command and address of the comelit simplebus2 protocol.

The automation first checks whether the length of the array is what we expect for a correctly received command, then the value of the address and command is decoded, the checksum check is performed, and if the checksum is correct, a home assistant event of the type "esphome.comelit_received" with command and address in the data field is generated.

It will be enough to create an automation on home assistant triggered by the event, discriminating only those with command "50" and address related to one's home station, in order to receive a notification when our intercom is called.

#### trigger example:

    platform: event
    event_type: esphome.comelit_received
    event_data:
      command: "50"
      address: "11"


In order to send commands over the bus, I have built a simple function that is invoked through a Home Assistant service, which converts the address and command fields into the array of integers that can be fed to the "remote transmitter" component.
Remote transmitter will take care of driving the transistor, modulating it with a 25kHz carrier for the number of microseconds specified in the integer values of the array.

The function that takes care of the encoding resides within the comelit.h file, which must be placed in the "esphome" folder of your home assistant configuration; it will be called via an include.

You can also make services with already internally coded command and address set previously. You can then create a convenient widget on your smartphone, which will simply invoke the "esphome.open_door" service so that you can open the door with a touch.

![Widget](/images/widget.jpg)

The system creates an event on the home assistant log for each command received on the bus. If you do not want to use this function, just delete the relevant part of the software.

![Log](/images/log.png)

The log is "hooked" to the sensor.comelit_state entity

## Installation

- Copy inside the esphome folder of home assistant the comelit.h file.
- Connect the wemos to the pc and add it to your esphome configuration. Do not connect it to the pc while it is bus powered.
- Make a note of the api encription key and ota password of the automatically generated configuration file.
- Replace the base code generated by esphome with the code in the esphome.yaml file.
- Replace the api encription key and ota password pinned previously in the appropriate fields.

## Purchase of materials and pcb

I have the ability to provide the pcb, components, or even the entire hardware already soldered and tested.

If you are interested, please contact me at mansellrace@gmail.com

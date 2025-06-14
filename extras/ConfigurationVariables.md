# Setting servo speed and end positions #
Configuration of servo speed and end positions (thresholds) can be performed via a normal DCC handheld. For that purpose, the decoder listens to the loco address: *7000 + Decoder address*.

Example: if the switch address on a Lenz LH100 is in the range of 397..400, the decoder address will be 99 (*400 / 4 - 1*) and the loco address 7099 (*7000 + 99*)). Note that this loco address can also be used for CV configuration via PoM.

![State diagram](StateMachineServoConfig.png "State diagram")

The figure above shows the state diagram to configure the servos.
To enter configuration mode, F9 should be pushed tree times on, and two times off, within a period of three seconds.

// - F9: enter / leave configuration mode
// - F1 .. F4: select servo 1 ..4
// - F5: Set servo in middle position, as well as both tresholds (1500 us).
// - F6: Set sevo speed (default = 6)
// - F7: Set treshold 1 (straight)
// - F8: Set treshold 2 (diverging)
// - F0: store current setting in EEPROM (while in F6, F7 or F8)


Links to further information:
- [Details of the decoder's operation](extras/Description.md#Description)
- [Used addresses and configuration](extras/Addresses.md#Addresses)
- [Alternatives to stop trains](extras/HowToStop.md#HowToStop)

### RS-Bus feedback ###
The servo decoder is able to send feedback information via the (Lenz) RS-Bus. In addition to sending feedbacks, this bus can also be used for reading CV values via PoM messages (RS-Bus address 128).

### Software ###
The servo decoder software is written for the Arduino IDE. The software requires the use of the AP_DCC_Decoder_Core: https://github.com/aikopras/AP_DCC_Decoder_Core, as well as the Servo-TCA library: https://github.com/aikopras/Servo-TCA.

### Hardware ###
The software runs on the servo-2 decoder board with a AVR64DA28 processor. The design of this board is open source, and it can be found on [OSHWLAB](https://oshwlab.com/aikopras/support-lift-controller_copy_copy_copy_copy). From there it can be imported into EasyEda and ordered at JLCPCB. The AVR64DA28 processor is a THT component, and can be ordered from companies such as Mouser.

A variant of this is being planned, and will also be made available via OSHWLAB.

![Servo-2 Board](extras/ServoPrint-2.jpeg "Servo-2 Board")

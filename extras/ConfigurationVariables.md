# Configuration Variables #

The first 63 CVs are generic CVs, and defined by the [AP_DCC_Decoder_Core](https://github.com/aikopras/AP_DCC_Decoder_Core/blob/main/src/CvValues/CvValues.md) library. CV64 shows the number of servos for this board.

The following CVs are specific for servos:
````
CVx+0   MinLow              Minimum servo position - low order byte
CVx+1   MinHigh             Minimum servo position - high order byte
CVx+2   MaxLow              Maximum servo position - low order byte
CVx+3   MaxHigh             Maximum servo position - high order byte
CVx+4   CurveA              Curve to be used for the A direction
CVx+5   CurveB              Curve to be used for the B (opposite) direction
CVx+6   Speed               time stretch (1..255)
CVx+7   InvertServoDir      Invert servo direction
CVx+8   InvertRelais        Invert polarisation relais
CVx+9   ServoType           0 = use the Pulse and Power CVx+s below, 1 = ...
CVx+10  PulseStartUpValue   Pulse signal during startup. 0 = low, 1 = high
CVx+11  PulseStartUpDelay   Duration of the startup pulse. In 20 ms ticks.
CVx+12  IdlePulseDefault    Pulse signal between moves (low, high / continuous pulse)
CVx+13  PulseOnBefore       Number of pulses before the servo starts moving
CVx+14  PulseOffAfter       Number of pulses after the servo has moved
CVx+15  PowerWhenIdle       0: power is off between servo movements
CVx+16  PowerOnBefore       In 20ms ticks
CVx+17  PowerOffAfter       In 20ms ticks
````
Per servo, 18 bytes are used. Thus for 2 servos this is 36 bytes, for 3 it is 54 and for 6 it is 108. The CVs for the first servo start at position CV65, for the second at position 83 (65+18) etc.

After the servo specific CVs there is space for 2 or 4 user-defined EEPROM curves. Each curve requires 48 bytes. If the total EEPROM size is 256 bytes, there is room for 2 curves. If the EEPROM is 512, there is room for 4 curves. See "Coding of curves" below for details.

### Invert ###
The Invert CV has consists of several parts:
- Bit 0: 1 = Switch position (straight/curved) should be inverted
- Bit 1: 1 = Relais position should be inverted. As default, the relais is inactive for a straight direction
- Bit 2: 1: The servo power enable pin is activated if LOW (normal case is HIGH)     

### CurveA ###
The bits within this CV has the following meaning:
- Bit 7 (MSB): Curve direction. 0 = normal direction, 1 = opposite direction (both Y coordinates are interchanged). In normal cases this bit is cleared (0).
- Bit 6: Curve storage location. 0 = curve is stored in PROGMEM, 1 = curve is stored in EEPROM. PROGMEM curves are predefined and cannot be changed by the user. EEPROM curves are user-defined. To define a new curve, the user should enter values in the CVs starting from 193 (or 129, in case the microcontroller has only 256 bytes of EEPROM). See "Coding of curves" below for details.
- Bits 5..0: index that points to the desired curve. The indexes for all PROGMEM curves are defined in the "Servo-TCA" library, in the file: [src/TCA_MobaCurves/curves.cpp]((https://github.com/aikopras/Servo-TCA/blob/main/src/TCA_MobaCurves/curves.cpp) (which is basically copied from the [OpenDCC project](https://www.opendcc.de/elektronik/opendecoder/opendecoder_sw_servo.html).

### CurveB ###
For CurveB, the same holds as for CurveA. However: CurveB may also have a value of 0 or 255. In that case CurveB will be the same as CurveA, except that the curve is traversed in opposite direction. This is called a symmetric curve. Another way to define a symmetric curve, is to enter for CurveB the same value as CurveA.

### Coding of curves ###
Each curve is defined by pairs of (time, position) values. The last pair must always be (0, 0). The maximum number of pairs (including the trailing (0, 0)) should not exceed 24. The format of these curves is the same as the [curves that are stored in flash memory](https://github.com/aikopras/Servo-TCA/blob/main/src/TCA_MobaCurves/curves.cpp). For an explanation of the time / position values, see also the [OpenDCC site](https://www.opendcc.de/elektronik/opendecoder/opendecoder_sw_servo.html).

### ServoTypec###
- 0: Generic servo. Uses values from CVs 10..17
- 1: Uhlenbrck standard-Servo (81420) / Weiner Mein Antrieb
- 2: MBTronic
- 3: SG90 - Tower Pro
- 4: SG90 - TZT

### PulseStartUpValue / PulseStartUpDelay ###
For most servos that were tested, the PulseStartUpValue should be HIGH.
A reasonable value for the PulseStartUpDelay would be 25 (500ms)

### IdlePulseDefault ###
The IdlePulseDefault CV has consists of several parts:
- Bit 1: 0: No pulse signal between servo movements (see bit 0) / 1: pulse signal remains active
- Bit 0: 0: Low pulse signal between servo movements / 1: high pulse signal (bit 1 should be 0)

### PulseOnBefore / PowerOnBefore ###
For the servos that were tested, both values should be 0, to avoid / limit the effect of a small, initial jump. With the servo provided by MBTronics, a small jump remained.

### PulseOffAfter / PowerOffAfter ###
To ensure that the servo always halts at the same position, it was important to keep the steps and power for a certain time. That time varied per servo, and could be 2 (40ms) but also 10 (200ms).

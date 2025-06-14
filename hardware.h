// *******************************************************************************************************
// File:      hardware.h
// Author:    Aiko Pras
// History:   2025/02/11 AP Version 1.0
//            2025/06/01 ap: first production version 
// 
// Purpose:   AVR Servo - Details for Board V2.0 (2022/07)
//  
// The AVR Servo decoder software has been developed for the AVR64DA28 processor. 
//
// Note that all DCC and RS-Bus pins are defined and initialised in the AP_DCC_Decoder_Basic library,
// which is included by dcc_rs.h
// PA0: RSBus RX
// PA4: RSBus TX
// PA7: LED (yellow)
// PD0: DCC IN
// PD1: DCC ACK
// PD2: TASTER
// 
// Compile settings:
// =================
// The following settings are used (under the Tools section of the Arduino IDE):
// - Board: AVR DA-series (no bootloader)  - Important to set this right
// - Chip: AVR64DA28                       - Important to set this right
// - Clock: 24 Mhz                         - Might work at other speeds
// - Millis: TCB2                          - Default
// - BOD: 1,9V                             - Default
// - EEPROM: ***                           - Seems that retained doesn't work
// - Startup time: 8ms                     - Default
// - FLMAP: Use last section               - Here we store PROGMEM variables
// - Port: usbserial....                   - Depends on the specifics of your development system
// - Programmer: Serial UPDI               - Depends on the specifics of your development system
//
// ******************************************************************************************************
#pragma once

// Number of Servos
// The number of servos can be set via "#define NUMBER_OF_SERVOS" below.
// Although the software is written to support multiple servos, the maximum amount is determined by:
// - The board (V2.0 - 2022/07 supports 2 Servos, V3.0 - 2025/XX supports 3 servos)
// - The number of TCA timers. Each TCA timer supporst upto 3 servos. The AVR64DA28 has a TCA timer.
// - The EEPROM size. Size = 256 => 4 servos / size = 512 => 8 servos
// - The RS-Bus code returns the positions of the first two servos only
#define NUMBER_OF_SERVOS      2


// In addition to the normal (DCC, RS-bus, LED, Taster) hardware, the AVR Servo decoder V2.0 has 
// the follwing specific hardware:
// - Two servo pulse outputs
// - Two Servo Enable pins 
// - Two relays (to change frog polarity)
// - Two pins for directly changing the switch position
// - A serial monitor interface
// - An extra LED (to indicate configuration mode)
// In addition, the following hardware can be connected via the IDC16 connector
// - Three pins to be used for rotary knobs
// - One extra input button
// - One SCA and one SCL pin

#define SERVO0_PIN            PIN_PF0
#define SERVO1_PIN            PIN_PF1
#define SERVO0_ENABLE_PIN     PIN_PA1
#define SERVO1_ENABLE_PIN     PIN_PA2
#define SERVO_ENABLE_VALUE    1         // servo gets powered with a high signal (board depended)

#define POSITION0_PIN         PIN_PA5
#define POSITION1_PIN         PIN_PA6
#define DEBOUNCE_TIME         100       // 100 ms

#define RELAYS0_PIN           PIN_PA3
#define RELAYS1_PIN           PIN_PD3

#define LED_CONFIG            PIN_PD4

#define ROTARY_A              PIN_PD5
#define ROTARY_BUTTON         PIN_PD5
#define ROTARY_B              PIN_PD7

#define Monitor               Serial1


//*****************************************************************************************************
// EEPROM specific settings and usage - Do not edit below!
//*****************************************************************************************************
//
//   0        1 .. 63       64             65...                                                    511
// +---+------------------+---+-----------------------------+----------------+-----------------------+
// | I |    CVs: 1..63    | # |   Servo specific CVs: 65..  |     Curves     |    Circular Buffer    |
// +---+------------------+---+-----------------------------+----------------+-----------------------+
//
// Contents of the EEPROM:
// - The first EEPROM byte indicates if the EEPROM has been initialised (the value 0b01010101)
// - The following 63 bytes hold the default CVs, as defined in "AP_DCC_Decoder_Core"
// - Byte 64 holds the number of servos for this board (see #define above)
// - The following bytes hold the servo specific CVs. Per servo, 18 bytes are used. Thus for 2 servos
//   this is 36 bytes, for 3 it is 54 and for 6 it is 108.
// - After the servo specific CVs there is space for 2 or 4 curves. Each curve requires 48 bytes
//   If the total EEPROM size is 256 bytes, we have room for 2 curves. If the EEPROM is 512, there
//   is room for 4 curves.
// - The last part of EEPROM space is used by the circular buffer. The goal of this buffer is
//   to improve EEPROM endurance. The first byte holds the number of boots, the other bytes 
//   store the most recently used servo curves / positions. See servo_position.h for details.
//
// Example for a 512 byte EEPROM and 2 servos:
// -       0: EEPROM has been initialized
// -    1-63: default CVs
// -      64: number of servos
// -   65-82: Servo-0 => 18 bytes
// -  83-100: Servo-1
// - 101-148: curve 0 => 48 bytes
// - 149-196: Default curve 1
// - 197-244: Default curve 2
// - 245-292: Default curve 4
// -     293: Number of Boots
// - 294-511: circular buffer for holding the last curve/direction => 216 bytes
//
// All EEPROM indexes will be automatically generated, once the NUMBER_OF_SERVOS and the EEPROM_SIZE
// are know. Therefore, do not change any of the #defines below. Note that it is important to embrace
// the strings from which the index values must be calculated by brackets!
//
//*****************************************************************************************************
#if (EEPROM_SIZE < 256) 
  #error At least 256 bytes of EEPROM needed!
#elif (EEPROM_SIZE < 512) 
  #define NUMBER_OF_CURVES 2
#else
  #define NUMBER_OF_CURVES 4
#endif

#define NUMBER_OF_SERVO_CVS            18

#define START_INDEX_SERVO_CVS          65
#define START_INDEX_SERVO_CURVES       (START_INDEX_SERVO_CVS + (NUMBER_OF_SERVOS * NUMBER_OF_SERVO_CVS))
#define EEPROM_BOOTS_INDEX             (START_INDEX_SERVO_CURVES + (NUMBER_OF_CURVES * 48))

#define TEMP_SIZE_CIRCULAR_BUFFER      (EEPROM_SIZE - EEPROM_BOOTS_INDEX - 1)

#if (TEMP_SIZE_CIRCULAR_BUFFER > 250)  // The stored circular buffer indexes are 8 bit. Avoid Overflow 
  #define SIZE_CIRCULAR_BUFFER         256
#else
  #define SIZE_CIRCULAR_BUFFER         TEMP_SIZE_CIRCULAR_BUFFER
#endif

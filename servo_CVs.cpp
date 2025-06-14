//*****************************************************************************************************
//
// File:      servo_CVs.cpp
// Author:    Aiko Pras
// History:   2025/03/22 
//            2025/06/01 ap: first production version 
// 
// Read, write and initialise the servo specific CVs.
//
//******************************************************************************************************
#include "servo_CVs.h"
#include "servo_position.h"
#include <AP_DCC_Decoder_Core.h>     // To use cvValues read and write 
#include <EEPROM.h>


uint8_t ReadServoCV(uint8_t servo, uint8_t CV) {
  uint16_t CvIndex = START_INDEX_SERVO_CVS + (servo * NUMBER_OF_SERVO_CVS) + CV;
  return EEPROM.read(CvIndex);
};


void WriteServoCV(uint8_t servo, uint8_t CV, uint8_t value) {
  uint16_t CvIndex = START_INDEX_SERVO_CVS + (servo * NUMBER_OF_SERVO_CVS) + CV;
  if (servo < NUMBER_OF_SERVOS) EEPROM.update(CvIndex, value); 
};


//******************************************************************************************************
// Support functions
//******************************************************************************************************
uint16_t ReadServoMin(uint8_t servo) {
  uint8_t valueLow = ReadServoCV(servo, MinLow);
  uint8_t valueHigh = ReadServoCV(servo, MinHigh);
  return valueHigh * 256 + valueLow;
};

uint16_t ReadServoMax(uint8_t servo) {
  uint8_t valueLow = ReadServoCV(servo, MaxLow);
  uint8_t valueHigh = ReadServoCV(servo, MaxHigh);
  return valueHigh * 256 + valueLow;
};

void WriteServoMin(uint8_t servo, uint16_t value) {
  uint8_t valueLow = value % 256;
  uint8_t valueHigh = value / 256;
  WriteServoCV(servo, MinLow, valueLow);
  WriteServoCV(servo, MinHigh, valueHigh); 
};

void WriteServoMax(uint8_t servo, uint16_t value) {
  uint8_t valueLow = value % 256;
  uint8_t valueHigh = value / 256;
  WriteServoCV(servo, MaxLow, valueLow);
  WriteServoCV(servo, MaxHigh, valueHigh); 
};


//******************************************************************************************************
// Initialisation of the servo specific CVs
// MinLow              =  0;  // Minimum servo position - low order byte
// MinHigh             =  1;  // Minimum servo position - high order byte
// MaxLow              =  2;  // Maximum servo position - low order byte
// MaxHigh             =  3;  // Maximum servo position - high order byte
// CurveA              =  4;  // Curve to be used for the A direction
// CurveB              =  5;  // Curve to be used for the B (opposite) direction (see above)
// Speed               =  6;  // time stretch (1..255)
// InvertServoDir      =  7;  // Invert servo direction
// InvertRelais        =  8;  // Invert polarisation relais
// ServoType           =  9;  // 0 = use the Pulse and Power CVs below, 1 = ...
// PulseStartUpValue   = 10;  // Pulse signal during startup. 0 = low, 1 = high
// PulseStartUpDelay   = 11;  // Duration of the startup pulse. In 20 ms ticks.
// IdlePulseDefault    = 12;  // Pulse signal between moves (low, high or continuous pulse)
// PulseOnBefore       = 13;  // Number of pulses before the servo starts moving
// PulseOffAfter       = 14;  // Number of pulses after the servo has moved
// PowerWhenIdle       = 15;  // 0: power is off between servo movements
// PowerOnBefore       = 16;  // In 20ms ticks
// PowerOffAfter       = 17;  // In 20ms ticks


//******************************************************************************************************
void CreateDefaultServoValuesInEEPROM() {
  // Step 1: Store the number of servos in the CV proeceeding the first servo CVs
  cvValues.write((START_INDEX_SERVO_CVS - 1), NUMBER_OF_SERVOS);
  // Step 2: Set the CV values for upto 8 servos
  for (uint8_t i = 0; i < NUMBER_OF_SERVOS; i++) {
    WriteServoMin(i, 1300);                         // in us 
    WriteServoMax(i, 1700);                         // in us
    WriteServoCV(i, CurveA, 2);                     // Smooth move for switches (250ms)
    // WriteServoCV(i, CurveA, 11);                 // Sinus, might be used for testing
    WriteServoCV(i, CurveB, 0);                     // 0 = Symmetric curve, CurveA in opposite direction
    WriteServoCV(i, Speed, 6);                      // 6 x 250 ms = 1,5 sec
    WriteServoCV(i, InvertServoDir, 0);             // Green = straight
    WriteServoCV(i, InvertRelais, 0);               // polarisation relais: not inverted
    WriteServoCV(i, ServoType, 0);                  // use the CVs below
    WriteServoCV(i, PulseStartUpValue, 1);          // After reboot, the pulse signal is High 
    WriteServoCV(i, PulseStartUpDelay, 25);         // After reboot, we delay by this value
    WriteServoCV(i, IdlePulseDefault, 1);           // High pulse signal betwen between moves
    WriteServoCV(i, PulseOnBefore, 0);              // 0 ms
    WriteServoCV(i, PulseOffAfter, 10);             // 200 ms
    WriteServoCV(i, PowerWhenIdle, 0);              // power enable signal will be low between moves
    WriteServoCV(i, PowerOnBefore, 0);              // 0 ms
    WriteServoCV(i, PowerOffAfter, 10);             // 200 ms
  };
  // Step 3: Clear the curves in EEPROM (all values become 0)
  uint16_t endIndexServoCurves = START_INDEX_SERVO_CURVES + (NUMBER_OF_CURVES * 48);
  for (uint16_t i = START_INDEX_SERVO_CURVES; i < endIndexServoCurves; i++) {
    EEPROM.update(i, 0);
  }  
  // Step 4: Clear the circular buffer (all values become 255)
  // Instead of writing such code again, we use the existing routine in servo_positions
  // To call that routine, we need to declare a local object first
  ServoPosition circularBuffer;
  circularBuffer.clearEEPROMCircularBufferValues();
};

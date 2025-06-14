// *****************************************************************************************************
//
// File:      servo_CVs.h
// Author:    Aiko Pras
// History:   2025/03/22 
//            2025/06/01 ap: first production version 
// 
// As opposed to some of my earlier decoders, the servo decoder needs more CVs to allow the user
// to change several aspects of the servo's behavior. Therefore the CV space is divided into two parts:
// 1) CV 1..63, which are the "standard" CVs that are declared by AP_DCC_Decoder_Core
// 2) CV 65.. for the servo specific CVs. 16 bytes per servo.
// See hardware.h for details. That file also includes the defines that tell where the servo specific
// CVs start in EEPROM.  
//
// Invert
// =======
// The Invert CV has consists of several parts:
// - Bit 0: 1 = Switch position (straight/curved) should be inverted
// - Bit 1: 1 = Relais position should be inverted
//          As default, the relais is inactive for a straight direction
// - Bit 2: 1: The servo power enable pin is activated if LOW (normal case is HIGH)     
//
// CurveA
// ======
// The bits within this CV has the following meaning:
// Bit 7 (MSB): Curve direction. 
//        0 = normal direction, 1 = opposite direction (both Y coordinates are interchanged)
//        In normal cases this bit is cleared (0).
// Bit 6: Curve storage location
//        0 = curve is stored in PROGMEM, 1 = curve is stored in EEPROM
//        PROGMEM curves are predefined and cannot be changed by the user
//        EEPROM curves are user-defined. To define a new curve, the user should enter values in the 
//        CVs starting from 193 (or 129, in case the microcontroller has only 256 bytes of EEPROM).
//        See "Coding of curves" below for details. 
// Bits 5..0: index that points to the desired curve
//        The indexes for all PROGMEM curves are defined in the "Servo-TCA" library, 
//        in the file: src/TCA_MobaCurves/curves.cpp (and copied from the OpenDCC project)
//        See also: https://www.opendcc.de/elektronik/opendecoder/opendecoder_sw_servo.html

// CurveB
// ======
// For CurveB, the same holds as for CurveA.
// However: CurveB may also have a value of 0 or 255. In that case CurveB will be the same as
// CurveA, except that the curve is traversed in oposite direction. This is called a symmetric curve.
// Another way to define a symmetric curve, is to enter for CurveB the same value as CurveA.
//
// Coding of curves
// ================
// Each curve is defined by pairs of (time, position) values. The last pair must always be (0, 0).
// The maximum number of pairs (including (0, 0) should not exceed 24. 
// For an explanation of the time / position values, 
// see: https://www.opendcc.de/elektronik/opendecoder/opendecoder_sw_servo.html  
//
// ServoType
// =========
// - 0: Generic servo. Uses values from CVs 10..17
// - 1: Uhlenbrck standard-Servo (81420) / Weiner Mein Antrieb
// - 2: MBTronic
// - 3: SG90 - Tower Pro
// - 4: SG90 - TZT
//
// PulseStartUpValue / PulseStartUpDelay
// =====================================
// For most servos that were tested, the PulseStartUpValue should be HIGH.
// A reasonable value for the PulseStartUpDelay would be 25 (500ms)
//
// IdlePulseDefault
// ================
// The IdlePulseDefault CV has consists of several parts:
// - Bit 1: 0: No pulse signal between servo movements (see bit 0) / 1: pulse signal remains active
// - Bit 0: 0: Low pulse signal between servo movements / 1: high pulse signal (bit 1 should be 0)
//
// PulseOnBefore / PowerOnBefore
// =============================
// For the servos that were tested, both values should be 0, to avoid / limit the effect of a small,
// initial jump. With the servo provided by MBTronics, a small jump remained.
//
// PulseOffAfter / PowerOffAfter
// =============================
// To ensure that the serv always halts at the same position, it was important to keep the steps and 
// power for a certain time. That time varied per servo, and could be 2 (40ms) but also 10 (200ms).
//
// ******************************************************************************************************
#pragma once
#include <Arduino.h>
#include "hardware.h"

// For the CurveA and CurveB CVs, and the previousCurve attribute
#define INDEX      0b00111111
#define CURVE      0b01111111               // bits 0..6 
#define EPROM      0b01000000
#define DIRECTION  0b10000000


// The EEPROM offset values for the various servo specific CVs
const uint8_t MinLow              =  0;  // Minimum servo position - low order byte
const uint8_t MinHigh             =  1;  // Minimum servo position - high order byte
const uint8_t MaxLow              =  2;  // Maximum servo position - low order byte
const uint8_t MaxHigh             =  3;  // Maximum servo position - high order byte
const uint8_t CurveA              =  4;  // Curve to be used for the A direction
const uint8_t CurveB              =  5;  // Curve to be used for the B (opposite) direction (see above)
const uint8_t Speed               =  6;  // time stretch (1..255)
const uint8_t InvertServoDir      =  7;  // Invert servo direction
const uint8_t InvertRelais        =  8;  // Invert polarisation relais
const uint8_t ServoType           =  9;  // 0 = use the Pulse and Power CVs below, 1 = ...
const uint8_t PulseStartUpValue   = 10;  // Pulse signal during startup. 0 = low, 1 = high
const uint8_t PulseStartUpDelay   = 11;  // Duration of the startup pulse. In 20 ms ticks.
const uint8_t IdlePulseDefault    = 12;  // Pulse signal between moves (low, high or continuous pulse)
const uint8_t PulseOnBefore       = 13;  // Number of pulses before the servo starts moving
const uint8_t PulseOffAfter       = 14;  // Number of pulses after the servo has moved
const uint8_t PowerWhenIdle       = 15;  // 0: power is off between servo movements
const uint8_t PowerOnBefore       = 16;  // In 20ms ticks
const uint8_t PowerOffAfter       = 17;  // In 20ms ticks


void CreateDefaultServoValuesInEEPROM();

uint8_t ReadServoCV(uint8_t servo, uint8_t CV);
uint16_t ReadServoMin(uint8_t servo);
uint16_t ReadServoMax(uint8_t servo);

void WriteServoCV(uint8_t servo, uint8_t CV, uint8_t value);
void WriteServoMin(uint8_t servo, uint16_t value);
void WriteServoMax(uint8_t servo, uint16_t value);

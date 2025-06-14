//*****************************************************************************************************
//
// File:      myServo.cpp
// Author:    Aiko Pras
// History:   2025/02/22 
//            2025/06/01 ap: first production version 
// 
// Extends the ServoMoba class with some extra functionality that we need for this decoder
// A maximum of 6 servo objects can be instantiated
//
// For a high level overview, see myServo.h 
//
// Initialisation
// ==============
// During init(), the values of the CVs called CurveA and CurveB will be copied to the attributes
// called curve0 and curve1. 
// 
// Performance of initCurveFromX
// =============================
// Calling initCurveFromX costs between 10 and 50 us, depending on the size of the curve.
// Note that reading curves from PROGMEM or EEPROM are likely equally expensive.
// Writing to EEPROM is, according to the AVR-DA datasheets, far more expensive and may require
// 10 ms per byte! However, such write is non-blocking, and takes the processor slightly over 1 us.
//
// Switch position
// ===============
// Note that there is a difference in semantics between the MSB of previousCurve / curve0 / curve1, 
// and the requested switch (servo) position. 
// According to RCN-213, the switch positions are:
//  0: diverging track / red / -   => we will use curve0
//  1: straight track / green / +  => we will use curve1
//
//
//******************************************************************************************************
#include <Arduino.h>
#include "myServo.h"
#include "servo_CVs.h"               // Servo specific CVs
#include "hardware.h"                // Pin and EEPROM definitions
#include "servo_position.h"          // Storage for the servo positions in EEPROM


void MyServo::init(uint8_t myNumber) {
  // Store this servo number in a private variable. From this number we also know the various
  // pins (puls, enable and relais) this servo object is using. 
  servoNumber = myNumber;
  //
  // Read from the servo CVs in EEPROM the treshold values for the min and max movements (in us)
  // Store (set) these values in the myServo object (this must be done before the curve is loaded)
  setTreshold1(ReadServoMin(servoNumber));
  setTreshold2(ReadServoMax(servoNumber));
  //
  // Copy the CVs CurveA and CurveB into curve0 and curve1, copy the stretch factor CV and invert
  // (if needed) the servo direction by changing both curves.
  copyCurveCVs();
  timeMultiplier = ReadServoCV(servoNumber, Speed);
  if (ReadServoCV(servoNumber, InvertServoDir)) invertServoDirection();
  //
  // Read from the circular EEPROM buffer the previous curve for this servo, and load it.
  previousCurve = storedPositions.servoPositions[servoNumber];
  loadCurve(previousCurve);
  //
  // Determine the initial pulse width 
  // If the curve has been traversed in opposite direction, we need to initialise using the first 
  // curve position. If it has been traversed in normal direction, we need to use the last position.
  uint16_t initialPulseWidth; 
  if (previousCurve & DIRECTION) initialPulseWidth = getFirstCurvePosition();
    else initialPulseWidth = getLastCurvePosition();
  //
  // First we configure all variables that relate to the pulse signal, and set that signal to an
  // initial value (high, low or continuous pulses). These variables are either stored in CV 10..14,
  // or predefined for the specific servo beeing used.
  configPulseSignal(initialPulseWidth);
  //
  // Second we configure all variables that relate to the power signal. These variables
  // are either stored in CV 15..17, or predefined for the specific servo beeing used.  
  configPowerSignal();  
  //
  // Now we can attach the servo
  attachMyServo();
  // Finally set the pin for the polarisation relay (if present) as output. 
  // Check if polarisation should be inverted, and sets the relay to its initial value.
  if (ReadServoCV(servoNumber, InvertRelais))  invertPolarisationRelay = true;
    else invertPolarisationRelay = false;
  initPolarisationRelay();
  // 
  // printInfoIni();  // For debugging
}


//******************************************************************************************************
void MyServo::set(uint8_t position) {
  // According to RCN-213, the switch positions are:
  //  0: diverging track / red / -   => we will use curve0
  //  1: straight track / green / +  => we will use curve1
  // Note: if the InvertServoDir CV is set, init has changed curve0 and curve1
  // Check if the servo is already at the requested position
  if ((position == 0) && (previousCurve == curve0)) return;
  if ((position == 1) && (previousCurve == curve1)) return;
  //
  // No, the servo is not at the requested position. But is it a symmetric curve?
  // For that, we compare the CURVE bits (0...6) of either curve0 or curve1 to that of previousCurve
  if ((previousCurve & CURVE) == (curve0 & CURVE))  // Symmetric curve?
    previousCurve ^= DIRECTION;                     // Toggle the DIRECTION bit (MSB)
  else {                                            // Curve is not symmetric
    if (position == 0) loadCurve(curve0);           // curve0 is for position 0
    else loadCurve(curve1);                         // curve1 is for position 1
  };
  uint8_t dir = (previousCurve & DIRECTION) >> 7;   // Determine the new direction
  moveServoAlongCurve(dir);                         // Moves the servo!
  storedPositions.saveServoPosition(servoNumber, previousCurve);
  setPolarisationRelay(position);
}

bool MyServo::getPosition() {
  if (previousCurve == curve0) return 0;
  else return 1;
}

//******************************************************************************************************
// Private support functions during initialisation
//******************************************************************************************************
void MyServo::configPulseSignal(uint16_t initialPulseWidth) {
  // Routine that reads the pulse related CVs and calls the servoTCA's library initPulse. 
  // The parameters for initPulse depend on the servo type, 
  switch (ReadServoCV(servoNumber, ServoType)) {
    case 1:  // Uhlenbrock Standard-Servo: Art. 81420 / Weinert Mein Antrieb
      pulseAfterReboot(HIGH, 10);
      initPulse(1, 0, 4, initialPulseWidth);
    break;
    case 2:  // MBTronic
      pulseAfterReboot(HIGH, 10);
      initPulse(1, 0, 10, initialPulseWidth);
    break;
    case 3:  // SG90 - Tower Pro
      pulseAfterReboot(HIGH, 10);
      initPulse(1, 0, 10, initialPulseWidth);
    break;
    case 4:  // SG90 - TZT
      pulseAfterReboot(HIGH, 10);
      initPulse(1, 0, 3, initialPulseWidth);
    break;
    default: // Use the values from the pulse CVs
      pulseAfterReboot(
        ReadServoCV(servoNumber, PulseStartUpValue),
        ReadServoCV(servoNumber, PulseStartUpDelay)
        );
      if (ReadServoCV(servoNumber, IdlePulseDefault) > 1)    // continuous pulse
        writeMicroseconds(initialPulseWidth);
      else {
      initPulse(
        (ReadServoCV(servoNumber, IdlePulseDefault) & 0x01), // 0 or 1 (= 3,3 / 5V)
        ReadServoCV(servoNumber, PulseOnBefore),             // 0..255 (in 20ms steps)
        ReadServoCV(servoNumber, PulseOffAfter),             // 0..255 (in 20ms steps)
        initialPulseWidth
        );
      };
    break;
  };
};


void MyServo::configPowerSignal() {
  // Routine that calls the servoTCA's library initPower (which in turn calls pinMode)
  // STEP 1: Set the variables that are needed for the initPower() call
  bool idlePowerIsOff;    // should the servo power (Enable signal) be switch off while idle?
  uint8_t powerOnBefore;  // 0.255. Steps are in 20 ms
  uint8_t powerOffAfter;  // 0.255. Steps are in 20 ms
  switch (ReadServoCV(servoNumber, ServoType)) {
    case 1:  // Uhlenbrock Standard-Servo: Art. 81420 / Weinert Mein Antrieb
      idlePowerIsOff = true;
      powerOnBefore = 0; 
      powerOffAfter = 4;
    break;
    case 2:  // MBTronic
      idlePowerIsOff = true;
      powerOnBefore = 0; 
      powerOffAfter = 10;
    break;
    case 3:  // SG90 - Tower Pro
      idlePowerIsOff = true;
      powerOnBefore = 0; 
      powerOffAfter = 10;
    break;
    case 4:  // SG90 - TZT
      idlePowerIsOff = true;
      powerOnBefore = 0; 
      powerOffAfter = 3;
    break;
    default:  // Use the values from the power related CVs
      idlePowerIsOff = !ReadServoCV(servoNumber, PowerWhenIdle);
      powerOnBefore = ReadServoCV(servoNumber, PowerOnBefore); 
      powerOffAfter = ReadServoCV(servoNumber, PowerOffAfter);
    break;
  };
  // STEP 2: Call initPower(), but only if the enable pin for has been defined in "hardware.h".
  // SERVO_ENABLE_VALUE is a board specific constant, and thus defined in "hardware.h" (and not a CV)
  switch (servoNumber) {
    case 0: 
      #ifdef SERVO0_ENABLE_PIN
        initPower(idlePowerIsOff, SERVO0_ENABLE_PIN, SERVO_ENABLE_VALUE, powerOnBefore, powerOffAfter);
      #endif
    break;
    case 1: 
      #ifdef SERVO1_ENABLE_PIN
        initPower(idlePowerIsOff, SERVO1_ENABLE_PIN, SERVO_ENABLE_VALUE, powerOnBefore, powerOffAfter);
      #endif
    break;
    case 2: 
      #ifdef SERVO2_ENABLE_PIN
        initPower(idlePowerIsOff, SERVO2_ENABLE_PIN, SERVO_ENABLE_VALUE, powerOnBefore, powerOffAfter);
      #endif
    break;
    case 3: 
      #ifdef SERVO3_ENABLE_PIN
        initPower(idlePowerIsOff, SERVO3_ENABLE_PIN, SERVO_ENABLE_VALUE, powerOnBefore, powerOffAfter);
      #endif
    break;
    case 4: 
      #ifdef SERVO4_ENABLE_PIN
        initPower(idlePowerIsOff, SERVO4_ENABLE_PIN, SERVO_ENABLE_VALUE, powerOnBefore, powerOffAfter);
      #endif
    break;
    case 5: 
      #ifdef SERVO5_ENABLE_PIN
        initPower(idlePowerIsOff, SERVO5_ENABLE_PIN, SERVO_ENABLE_VALUE, powerOnBefore, powerOffAfter);
      #endif
    break;
  };
};


void MyServo::attachMyServo() { 
  // hardware.h is checked if the pin to which this servo should be attached, has been defined.
  // In this way we ensure that this (part of the) code runs for different numbers of servos.
  switch (servoNumber) {
    case 0: 
      #ifdef SERVO0_PIN
        attach(SERVO0_PIN); 
      #endif
    break;
    case 1: 
      #ifdef SERVO1_PIN
        attach(SERVO1_PIN); 
      #endif
    break;
    case 2: 
      #ifdef SERVO2_PIN
        attach(SERVO2_PIN); 
      #endif
    break;
    case 3: 
      #ifdef SERVO3_PIN
        attach(SERVO3_PIN); 
      #endif
    break;
    case 4: 
      #ifdef SERVO4_PIN
        attach(SERVO4_PIN); 
      #endif
    break;
    case 5: 
      #ifdef SERVO5_PIN
        attach(SERVO5_PIN); 
      #endif
    break;
  };
};


void MyServo::initPolarisationRelay() { 
  // hardware.h is checked if the pin to which this relay should be attached, has been defined.
  // In this way we ensure that this (part of the) code runs for different numbers of servos.
  switch (servoNumber) {
    case 0: 
      #ifdef RELAYS0_PIN
         pinMode(RELAYS0_PIN, OUTPUT);
      #endif
    break;
    case 1: 
       #ifdef RELAYS1_PIN
         pinMode(RELAYS1_PIN, OUTPUT);
      #endif
    break;
    case 2: 
       #ifdef RELAYS2_PIN
         pinMode(RELAYS2_PIN, OUTPUT);
      #endif
    break;
    case 3: 
       #ifdef RELAYS3_PIN
         pinMode(RELAYS3_PIN, OUTPUT);
      #endif
    break;
    case 4: 
       #ifdef RELAYS4_PIN
         pinMode(RELAYS4_PIN, OUTPUT);
      #endif
    break;
    case 5: 
       #ifdef RELAYS5_PIN
         pinMode(RELAYS5_PIN, OUTPUT);
      #endif
    break;
  };
  // Now we have to set the polarisation relay to its initial position
  // This is done by checking the DIRECTION bit in previousCurve 
  bool initialPosition = previousCurve & DIRECTION;
  if (servoDirectionInverted) initialPosition = !initialPosition;
  setPolarisationRelay(initialPosition);
};
  

void MyServo:: copyCurveCVs() {
  // Copy the values of the CVs CurveA and CurveB to the attributes curve0 and curve1
  // If the CV called CurveB has the value 0 or 255, the attribute curve1 will be copied from CurveA,
  // except that the direction (MSB) will be changed. This case is called a symmetric curve.
  // Another way to define a symmetric curve, is to give CurveB the same value as CurveA.
  // Also in that case the direction (MSB) will be changed.
  curve0 = ReadServoCV(servoNumber, CurveA);
  curve1 = ReadServoCV(servoNumber, CurveB);
  if ((curve1 == 0) || (curve1 == 255)) {         // Curve1 not initialised
    curve1 = curve0;                              // make curve1 symmetric, by copying curve0
    curve1 ^= 0x80;                               // and toggling the MSB
  }
  if (curve1 == curve0) curve1 ^= 0x80;           // Curve 1 same as curve 0 => Toggle MSB
};


void MyServo::invertServoDirection() {
  uint8_t temp = curve0;
  curve0 = curve1;
  curve1 = temp;
  servoDirectionInverted = !servoDirectionInverted;
}


void MyServo::loadCurve(uint8_t curve) {
  // May be called to set new speed 
  uint8_t curveNumber = curve & INDEX;            // EEPROM: 0, 1, 2 or 3 / PROGMEM: 
  if (curve & EPROM) {                            // EEPROM bit is set??
    if (curveNumber < NUMBER_OF_CURVES) {         // Protection, in case an erroneous CV value was entered
      uint16_t startAdres = START_INDEX_SERVO_CURVES + (curveNumber * 48);
      initCurveFromEEPROM(curve, timeMultiplier, startAdres);
    }
  }
  else
    if (curveNumber <= NUMBER_OF_LAST_CURVE) {    // Protection, in case an erroneous CV value was entered
    initCurveFromPROGMEM(curve, timeMultiplier);
  }
};



//******************************************************************************************************
// Private support functions during operation
//******************************************************************************************************
void MyServo::setPolarisationRelay(bool frogPosition) {
  // - OFF (without power): straigt / green / + / frogPosition = 1
  // - ON  (powered):     diverbing / red   / - / frogPosition = 0
  // This can be inverted via the CV InvertRelais / boolean: invertPolarisationRelay  
  // Thus, if the values of frogPosition and invertPolarisationRelay match, the relay should
  // be activated.
  boolean activateRelay = (invertPolarisationRelay == frogPosition);
  // hardware.h is checked if the pin to which this relay should be attached, has been defined.
  // In this way we ensure that this (part of the) code runs for different numbers of servos.
  // To avoid the overhead of the standard Arduino digitalWrite() routine, we use digitalWriteFast()
  // See: https://github.com/SpenceKonde/DxCore/blob/master/megaavr/extras/Ref_Digital.md
  switch (servoNumber) {
    case 0: 
      #ifdef RELAYS0_PIN
        digitalWriteFast(RELAYS0_PIN, activateRelay);
      #endif
    break;
    case 1: 
       #ifdef RELAYS1_PIN
        digitalWriteFast(RELAYS1_PIN, activateRelay);
      #endif
    break;
    case 2: 
       #ifdef RELAYS2_PIN
        digitalWriteFast(RELAYS2_PIN, activateRelay);
      #endif
    break;
    case 3: 
       #ifdef RELAYS3_PIN
        digitalWriteFast(RELAYS3_PIN, activateRelay);
      #endif
    break;
    case 4: 
       #ifdef RELAYS4_PIN
        digitalWriteFast(RELAYS4_PIN, activateRelay);
      #endif
    break;
    case 5: 
       #ifdef RELAYS5_PIN
        digitalWriteFast(RELAYS5_PIN, activateRelay);
      #endif
    break;
  };
};


void MyServo::pulseAfterReboot(uint8_t level, uint8_t waitTime) {
  // Set the pulse signal to a high or low level, and keep that level for a certain time
  constantOutput(level);   // 0 = LOW (0V), 1 = HIGH (3,3 or 5V)
  delay(waitTime * 20);    // waitTime is in 20ms ticks
}


void MyServo::printInfoIni() {
  Monitor.print("Servo: "); Monitor.print(servoNumber);
  Monitor.print(" - curve0: "); Monitor.print(curve0);
  Monitor.print(" - curve1: "); Monitor.print(curve1);
  Monitor.print(" - previousCurve: "); Monitor.println(previousCurve);
};

void MyServo::printInfoSet() {
  Monitor.print("New curve:"); Monitor.println(previousCurve);
  Monitor.print(" - direction: ");
  Monitor.print(previousCurve & DIRECTION);
  Monitor.print(" - previousCurve: ");
  Monitor.println(previousCurve);
}

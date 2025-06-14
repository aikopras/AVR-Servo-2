// *****************************************************************************************************
//
// File:      configure.h
// Author:    Aiko Pras
// History:   2025/05/31
//            2025/06/01 ap: first production version 
// 
// Purpose:   Configuration of the servo speed and Tresholds
//
//
// *****************************************************************************************************
#include <Arduino.h>                        // For general definitions
#include <AP_DCC_Decoder_Core.h>            // For the BasicLed class
#include "hardware.h"
#include "configure.h"
#include "MyServo.h"                        // Inherits and extends the ServoMoba class
#include "servo_CVs.h"

extern MyServo servo[NUMBER_OF_SERVOS];     // Should be instantiated in main()
extern BasicLed configLed;

#define F0  (bitRead(F0F4, 4))
#define F1  (bitRead(F0F4, 0))
#define F2  (bitRead(F0F4, 1))
#define F3  (bitRead(F0F4, 2))
#define F4  (bitRead(F0F4, 3))
#define F5  (bitRead(F5F8, 0))
#define F6  (bitRead(F5F8, 1))
#define F7  (bitRead(F5F8, 2))
#define F8  (bitRead(F5F8, 3))
#define F9  (bitRead(F9F12, 0))


// *****************************************************************************************************
// To enter configuration mode
// To enter configuration mode, push F9 multiple times on and off, within a certain time interval.
// The number of times is determined by the F9_ATTEMPTS_REQUIRED define. The interval in which these 
// pushes are needed, is given by the F9_ATTEMPTS_INTERVAL define. Time is in milliseconds. If this
// time is exceeded, reinitialise to start over.
// The last push should be F9 = ON, to ensure config mode is left once the handheld shows F9 = OFF 
// *****************************************************************************************************
bool Configure::checkStart(uint8_t value) {
  F9F12 = value;
  if (F9 && (F9_AttemptsLeft == F9_ATTEMPTS_REQUIRED)) {
    // F9 is pushed ON for the first time.
    F9_TimeFirstPush = millis();
    return 0;
  }
  // is this push still within the time interval
  if ((millis() - F9_TimeFirstPush) > F9_ATTEMPTS_INTERVAL) {
    // Too long. Reinitialise to allow a next configuration attempt
    F9_TimeFirstPush = millis();
    F9_AttemptsLeft = F9_ATTEMPTS_REQUIRED;
    return 0;
  }
  if (F9_AttemptsLeft == 0) {                 // switch to config mode
    configState = Ready;
    configLed.turn_on();
    return 1;
  }
  if (!F9) {                // F9 == OFF?
    F9_AttemptsLeft = F9_AttemptsLeft - 1;
  }
  return 0;
}


// *****************************************************************************************************
// Called from the main loop, once we are in config mode
// *****************************************************************************************************
void Configure::setF0F4(uint8_t value) {F0F4 = value;}
void Configure::setF5F8(uint8_t value) {F5F8 = value;}
void Configure::setF9F12(uint8_t value) {F9F12 = value;}


void Configure::setSpeed(uint8_t speed, bool dir) {
  locoSpeed = speed;
  locoDirection = dir;                // True = Forward / False = Reverse
}


bool Configure::checkConfig() {
  // Runs the state machine
  switch (configState) {
    case Waiting:  
    break;
    case Ready: return doReady(); // Should we stay in config mode?
    break;
    case Verify: doVerify();
    break;
    case Set: doSet();
    break;
    case Middle: doMiddle();
    break;
    case Speed: doSpeed();
    break;
    case TresholdStraight: doTresholdStraight();
    break;
    case TresholdDiverging: doTresholdDiverging();
    break;
  };
  return 1; // stay in config mode
}


// *****************************************************************************************************
// Local methods: State machine states 
// *****************************************************************************************************
bool Configure::doReady() {
  // Listens to F9, to leave configuration mode.
  if (!F9) {
    F9_AttemptsLeft = F9_ATTEMPTS_REQUIRED;      // reinitialise to allow a next configuration attempt
    configState = Waiting;
    configLed.turn_off();
    return 0;  // we leave configuration mode
  } 
  // Listens to F1 .. F4, to determine which servo we are operating on
  selectedServo = validServoFromF();
  if (selectedServo < NUMBER_OF_SERVOS) {
    configState = Verify; 
    numberOfMoves = NUMBER_OF_SERVO_MOVES;
  }
  return 1;  // We stay in configuratio mode
}


void Configure::doVerify() {
  // We move the selected servo a couple of times, to verify the right one is selected
  // Instead of set(), we use moveServo(), to avoid the relais from switching on and off
  if (numberOfMoves > 0) {
    if (servo[selectedServo].movementCompleted == true) {
      moveServo(selectedServo);
      numberOfMoves = numberOfMoves - 1;
      if (numberOfMoves == 0) configState = Set;
    }
  }
  else  // numberOfMoves == 0
    if (servo[selectedServo].movementCompleted == true) configState = Set;
}


void Configure::doSet() {
  if (servoDeselected()) {
    servo[selectedServo].configPowerSignal();                     // Restore the original idle power values from the CVs
    configState = Ready;
  }
  if (F5) {
    configState = Middle;
  }
  if (F6) {
    configState = Speed;
  }
  if (F7) {
    servo[selectedServo].set(1);
    if (servo[selectedServo].movementCompleted == true) configState = TresholdStraight;
  }
  if (F8) {
    servo[selectedServo].set(0);
    if (servo[selectedServo].movementCompleted == true) configState = TresholdDiverging;
  }
}


void Configure::doMiddle() {
  servo[selectedServo].powerOn();       // writeMicroseconds() requires power
  if (F5) { 
    servo[selectedServo].writeMicroseconds(1500);
    servo[selectedServo].setTreshold1(1500);
    servo[selectedServo].setTreshold2(1500);
  }
  else configState = Set;  // Return
}


void Configure::doSpeed() {
  if (servo[selectedServo].movementCompleted == true) {
    if (F6) {
      // The attribute timeMultiplier represents the time stretch factor for a curve.
      // To activate this timeMultiplier, a new curve must be loaded. To decide which
      // curve to load, we compare previousCurve with curveA and curveB 
      uint8_t stretch = locoSpeed;
      if (stretch < 1) stretch = 6;  // the default value
      servo[selectedServo].timeMultiplier = stretch;     
      servo[selectedServo].loadCurve(servo[selectedServo].previousCurve);
      if (servo[selectedServo].getPosition()) servo[selectedServo].set(0);
        else servo[selectedServo].set(1);
      if (F0) WriteServoCV(selectedServo, Speed, stretch);
    } 
    else {
      servo[selectedServo].timeMultiplier = ReadServoCV(selectedServo, Speed); // Restore EEPROM values
      servo[selectedServo].loadCurve(servo[selectedServo].previousCurve);
      configState = Set;  // Return
    }
  }
}


void Configure::doTresholdStraight() {  // Treshold 2
  servo[selectedServo].powerOn();       // writeMicroseconds() requires power
  if (F7) {
    if ((millis() - lastConfigTime) > CONFIG_STEP_TIME) {
      lastConfigTime = millis();
      uint16_t treshold = servo[selectedServo].getTreshold2();
      bool servoDirection = (servo[selectedServo].previousCurve & DIRECTION);
      if (servoDirection) currentPulseWidth = servo[selectedServo].getFirstCurvePosition();
      else currentPulseWidth = servo[selectedServo].getLastCurvePosition();
      if (locoSpeed < 10) {
        if (locoDirection) treshold = treshold + locoSpeed;
        else treshold = treshold - locoSpeed;
      }
      if ((treshold >= MIN_PULSE_WIDTH) && (treshold <= MAX_PULSE_WIDTH)) {
        servo[selectedServo].writeMicroseconds(treshold);
        servo[selectedServo].setTreshold2(treshold);
        if (F0) WriteServoMax(selectedServo, treshold);
      }
    }
  } else configState = Set;  // Return
}


void Configure::doTresholdDiverging() {  // Treshold 1
  servo[selectedServo].powerOn();        // writeMicroseconds() requires power
  if (F8) {
    if ((millis() - lastConfigTime) > CONFIG_STEP_TIME) {
      lastConfigTime = millis();
      uint16_t treshold = servo[selectedServo].getTreshold1();
      bool servoDirection = (servo[selectedServo].previousCurve & DIRECTION);
      if (servoDirection) currentPulseWidth = servo[selectedServo].getFirstCurvePosition();
        else currentPulseWidth = servo[selectedServo].getLastCurvePosition();
      if (locoSpeed < 10) {
        if (locoDirection) treshold = treshold - locoSpeed;
        else treshold = treshold + locoSpeed;
      }
      if ((treshold >= MIN_PULSE_WIDTH) && (treshold <= MAX_PULSE_WIDTH)) {
        servo[selectedServo].writeMicroseconds(treshold);
        servo[selectedServo].setTreshold1(treshold);
        if (F0) WriteServoMin(selectedServo, treshold);
      }
    }
  } else configState = Set;  // Return
}


// *****************************************************************************************************
// Local methods: support
// *****************************************************************************************************
uint8_t Configure::validServoFromF() {
  // Checks if one and only one of the functions F1 .. F4 is selected
  uint8_t Fs =  ((F5F8 & 0b00000000) << 4) + (F0F4 & 0b00001111);
  uint8_t result;
  switch (Fs) {
    case   1: result = 0; break;
    case   2: result = 1; break;
    case   4: result = 2; break;
    case   8: result = 3; break;
    default: result = 255; break;
  };
  // Is the servo associated with the selected function implemented on this board?
  if (result < NUMBER_OF_SERVOS) return result;
    else return 255;
};


bool Configure::servoDeselected() {
  switch (selectedServo) {
    case 0: if (F1) {return 0;} else return 1;
    case 1: if (F2) {return 0;} else return 1;
    case 2: if (F3) {return 0;} else return 1;
    case 3: if (F4) {return 0;} else return 1;
    default: return 0;
  };
}


void Configure::moveServo(uint8_t i) {
  servo[i].previousCurve ^= DIRECTION;                       // Toggle the DIRECTION bit (MSB)  
  uint8_t dir = (servo[i].previousCurve & DIRECTION) >> 7;   // Determine the new direction
  servo[i].moveServoAlongCurve(dir);                         // Moves the servo!
}
//*****************************************************************************************************
//
// File:      servo_position.h
// Author:    Aiko Pras
// History:   2025/02/13
//            2025/03/22   ap indexPosition0 and indexPosition1 moved into an Array
//            2025/06/01 ap: first production version 
// 
// How to store the current switch / servo position(s) in EEPROM, in such way that the wear-out
// gets reduced / EEPROM endurance gets improved. 
//
// EEPROM endurance for most AVR processors is specified to be 100.000. This number is guaranteed
// under all operational curcumstances. Lower temperature, or lower voltage, increases this number.
// Wear-out is specified at byte level, and states how often an individual byte can be (re)written. 
// If a value is stored in a circular buffer with a size of 128, the endurance is increased with a
// factor 128.
// 
// This is exactly the idea that we follow here. The key question, however, is how to know which 
// cell in the circular buffer holds the switch position (thus holds an index into the buffer).  
//
// The core idea is to have a location in EEPROM, called numberOfBoots, that holds a pointer
// to the cell(s) in the circular buffer that hold(s) the latest servo position(s). 
// Every time the decoder starts (resets, power-up), the numberOfBoots gets incremented by one.
// Once the decoder has started, the servo position(s) is(are) always written to the same  
// byte(s) in the circular buffer. The assumption is that the decoder is started only once a day, 
// but that during that day the servo(s) change their position many times. 
//
// Below the basic algorithm in more detail.
// At start-up, as part of setup(), the value of numberOfBoots is read from EEPROM, as well as the
// contents of the circular buffer location(s) that numberOfBoots points to (the servo position).
// NumberOfBoots gets updated after the first EEPROM write, thus after the first accessory command
// that leads to a change in position of a servo. The idea to update numberOfBoots after the first
// accessory command, and not as part of setup(), is to avoid unnecessary EEPROM writes in case the
// decoder is powered on, but not being used for servo movements.
//
// NumberOfBoots, and the circular buffer itself, will be stored at the end of the EEPROM space.
// The file "hardware.h" defines SIZE_CIRCULAR_BUFFER as well as EEPROM_BOOTS_INDEX. These values
// depend on the size of the EEPROM, and the number of supported servos for this board.
//
// If multiple servos are implemented, we store for each servo its positions.
// The position for the first servo is stored at the index where the contents of EEPROM_BOOTS_INDEX 
// points to. The subsequent servo position are stored in the bytes above.
// 
// Once the end of the circular buffer is reached, we continue at the start of the circular buffer.
// 
//******************************************************************************************************
#pragma once
#include <Arduino.h>
#include "hardware.h"


class ServoPosition {
  public:
    ServoPosition();                            // constructor

    void saveServoPosition(uint8_t number, uint8_t value); // should be called after a change
    uint16_t servoPositions[NUMBER_OF_SERVOS];  // Where are the various servo positions stored in EEPROM?
    
    void clearEEPROMCircularBufferValues();     // Can be called if the EEPROM gets (re)initialised
    void printEEPROM();                         // Only for testing
    
  private:
    bool firstCall;                             // The processor just got powered up / reset / reflashed
    void incrementNumberOfBoots();              // Will be called after the first servo position change
    void calculateservoPositions();             // Use numberOfBoots to find servoPositions[]
    uint16_t getIndex(uint8_t servoNumber);     // Calculate the storage position for a specific servo  

    uint8_t numberOfBoots;                      // 0 .. SIZE_CIRCULAR_BUFFER - 1
};


//******************************************************************************************************
// The object is defined in servo_position.cpp and may be used by myServo 
extern ServoPosition storedPositions;

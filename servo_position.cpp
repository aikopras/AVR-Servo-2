//*****************************************************************************************************
//
// File:      servo_position.cpp
// Author:    Aiko Pras
// History:   2025/03/22 
//            2025/06/01 ap: first production version 
// 
// To get a high-level understanding of what this code is supposed to do, see servo_position.h.
//
// To make the code easier to read (and thus reduce the chance we make mistakes), we maintain
// an array of (16 bit) indexes into the EEPROM that point to the EEPROM bytes that contain the values
// for the servo positions.  
//
// incrementNumberOfBoots() is called only once, when the first servo movement takes place.
//
//******************************************************************************************************
#include "servo_position.h"
#include <EEPROM.h>

// Instantiate the object for the stored positions
ServoPosition storedPositions;


ServoPosition::ServoPosition() {                        // Constructor. Called at start up
  firstCall = true;                                     
  numberOfBoots = EEPROM.read(EEPROM_BOOTS_INDEX);      // 1 .. SIZE_CIRCULAR_BUFFER
  if (numberOfBoots == 0) numberOfBoots = 1;            // the EEPROM is erased, and nothing is stored yet.
  if (numberOfBoots == 255) numberOfBoots = 1;          // the EEPROM is new, and nothing is stored yet.
  calculateservoPositions();                            // the index positions point to the old values
};


uint16_t ServoPosition::getIndex(uint8_t servoNumber) { 
  // Determines where the position for a specific servo is stored
  // numberOfBoots >= 1 / servoNumber = 0...
  uint16_t index = EEPROM_BOOTS_INDEX + numberOfBoots + servoNumber;    // EEPROM location for this servo
  if (index >= EEPROM_SIZE) index = index - SIZE_CIRCULAR_BUFFER;       // Wrap around
  return index;
};


void ServoPosition::incrementNumberOfBoots() {          // Will be called after first setPosition
  firstCall = false;                                    // make sure we are only called once!
  // We have to move all servo positions one index to the right
  // We first move the last (right most); the first position is moved last.
  // Note that the for statement counts down, and the last servoNr is 1 (not 0)
  for (uint8_t servoNr = NUMBER_OF_SERVOS; servoNr > 0; servoNr--) 
    EEPROM.update(getIndex(servoNr), EEPROM.read(getIndex(servoNr -1)));
  if (numberOfBoots == SIZE_CIRCULAR_BUFFER)            // Overflow??
    numberOfBoots = 1;                                  // then wrap around
    else numberOfBoots++;                               // else move the buffer index one byte further
  EEPROM.update(EEPROM_BOOTS_INDEX, numberOfBoots);     // Save the new numberOfBoots 
  calculateservoPositions();                            // The index positions point now to the new values 
};


void ServoPosition::calculateservoPositions() {
  // servoNr 0 .. (NUMBER_OF_SERVOS - 1)
  uint8_t value;
  for (uint8_t servoNr = 0; servoNr < NUMBER_OF_SERVOS; servoNr++) {
    value = EEPROM.read(getIndex(servoNr));
    if (value == 255) value = 0;                        // EEPROM has not yet been initialsed
    servoPositions[servoNr] = value;
  };
};


void ServoPosition::saveServoPosition(uint8_t number, uint8_t value) {
  if (firstCall) incrementNumberOfBoots();
  EEPROM.update(getIndex(number), value);
  // The lines below are for debugging only
  // Monitor.print("saveServoPosition. servo: "); Monitor.print(number);
  // Monitor.print(" - index: "); Monitor.print(getIndex(number));
  // Monitor.print(" - value: "); Monitor.println(value);
};


void ServoPosition::clearEEPROMCircularBufferValues() {
  uint16_t i = EEPROM_BOOTS_INDEX;
  while (i < EEPROM_SIZE) {
    EEPROM.update(i, 255);
    i++;
  };
  firstCall = true; 
  numberOfBoots = 1;                                    // the EEPROM is new, and nothing is stored yet.
  calculateservoPositions();                            // initialise these
}


//******************************************************************************************************
void ServoPosition::printEEPROM() {                     // For debugging
  // Monitor.print("");
  Monitor.print("EEPROM_SIZE: ");
  Monitor.println(EEPROM_SIZE);
  Monitor.print("EEPROM Boots index: ");
  Monitor.println(EEPROM_BOOTS_INDEX);
  Monitor.print("Number of Boots: ");
  Monitor.println(numberOfBoots);
  Monitor.print("indexPosition 0: ");
  Monitor.println(getIndex(0));
  Monitor.print("indexPosition 1: ");
  Monitor.println(getIndex(1));
  Monitor.print("Servo0 position: ");
  Monitor.println(servoPositions[0]);
  Monitor.print("Servo1 position: ");
  Monitor.println(servoPositions[1]);
};

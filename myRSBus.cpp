// *****************************************************************************************************
//
// File:      myRSBus.cpp
// Author:    Aiko Pras
// History:   2025/05/05
//            2025/06/01 ap: first production version 
// 
// Purpose:   Implementation of RS-Bus feedback functions
//
// The first set of methods set the bits in the respective nibble, and send that nibble
// via the RS-Bus connection. In addition, they update the 8-bit feedback data, which is
// needed after the RS-Bus connection was temporary lost 
//
// The init() method must be called after the servos got attached, since it needs to know  
// the positions of the servos after startup
//
// *****************************************************************************************************
#include <Arduino.h>                        // For general definitions
#include "myRSBus.h"
#include "servo_CVs.h"                      // for the #define DIRECTION
#include "MyServo.h"                        // Class definition, to access the servo objects from main


// We need to be able to access some objects from main()
extern MyServo servo[NUMBER_OF_SERVOS];


// *****************************************************************************************************
void MyRsBus::sendNibble0(uint8_t position) {
  if (position) feedbackNibble0 = 0b00001010;
  else feedbackNibble0 = 0b00000101;
  send4bits(LowBits, feedbackNibble0);
  feedback8Bit &= ~0b00001111;                  // clear nibble 0
  feedback8Bit |= feedbackNibble0;              // Add nibble 0
}

void MyRsBus::sendNibble1(uint8_t position) {
  if (position) feedbackNibble1 = 0b00001010;
  else feedbackNibble1 = 0b00000101;
  send4bits(HighBits, feedbackNibble1);
  feedback8Bit &= ~0b11110000;                  // clear nibble 1
  feedback8Bit |= (feedbackNibble1 * 16);       // Add nibble 1
}


void MyRsBus::sendFB01(uint8_t position) {
  feedbackNibble0 &= ~0b00000011;               // clear bits 0 and 1
  if (position) feedbackNibble0 |= (0x01 << 1); // set bit 1
  else feedbackNibble0 |= (0x01 << 0);          // set bit 0
  send4bits(LowBits, feedbackNibble0);             
  feedback8Bit &= ~0b00001111;                  // clear nibble 0
  feedback8Bit |= feedbackNibble0;              // Add nibble 0
}

void MyRsBus::sendFB23(uint8_t position) {
  feedbackNibble0 &= ~0b00001100;               // clear bits 0 and 1
  if (position) feedbackNibble0 |= (0x01 << 3); // set bit 3
  else feedbackNibble0 |= (0x01 << 2);          // set bit 2
  send4bits(LowBits, feedbackNibble0);             
  feedback8Bit &= ~0b00001111;                  // clear nibble 0
  feedback8Bit |= feedbackNibble0;              // Add nibble 0
}

void MyRsBus::sendFB45(uint8_t position) {                // xx10xxxx or xx01xxxx
  feedbackNibble1 &= ~0b00000011;               // clear bits 0 and 1
  if (position) feedbackNibble1 |= (0x01 << 1); // set bit 1
  else feedbackNibble1 |= (0x01 << 0);          // set bit 0
  send4bits(HighBits, feedbackNibble1);             
  feedback8Bit &= ~0b11110000;                  // clear nibble 1
  feedback8Bit |= (feedbackNibble1 * 16);       // Add nibble 1
}

void MyRsBus::sendFB67(uint8_t position) {                // 10xxxxxx or 01xxxxxx
  feedbackNibble1 &= ~0b00001100;               // clear bits 0 and 1
  if (position) feedbackNibble1 |= (0x01 << 3); // set bit 3
  else feedbackNibble1 |= (0x01 << 2);          // set bit 2
  send4bits(HighBits, feedbackNibble1);             
  feedback8Bit &= ~0b11110000;                  // clear nibble 1
  feedback8Bit |= (feedbackNibble1 * 16);       // Add nibble 1
}


void MyRsBus::checkRSFeedback() {
  // Should be called from main as frequent as possible
  // As frequent as possible we should check if the RS-Bus asks for the most recent feedback data.
  // This is the case after a decoder restart or after a RS-Bus error. In addition, we have to 
  // check if the buffer contains feedback data, and the ISR is ready to send that data via the UART.  
  if (feedbackRequested) send8bits(feedback8Bit);
  checkConnection();
}

// ******************************************************************************************************
// Initialisation and local routines
// ******************************************************************************************************
void MyRsBus::init(uint8_t RSBusAddress, bool skipUnEven) {
  // Must be called after the servos are attached
  address = RSBusAddress;
  feedbackNibble0 = setNibble0(skipUnEven);
  feedbackNibble1 = setNibble1(skipUnEven);
  feedback8Bit = (feedbackNibble1 * 16) + feedbackNibble0;
}


uint8_t MyRsBus::setNibble0(uint8_t skipUnEven) {
  uint8_t result = 0;
  if (skipUnEven) {
    if (servo[0].previousCurve & DIRECTION) result = 0b00001010;
    else result = 0b00000101;
  }
  else {
    if (servo[0].previousCurve & DIRECTION) result |= (0x01 << 1);  // set bit 1;
    else result |= (0x01 << 0);                                   // set bit 0;
    if (servo[1].previousCurve & DIRECTION) result |= (0x01 << 3);  // set bit 1;
    else result |= (0x01 << 2);                                   // set bit 0;
  };
  return result;
}


uint8_t MyRsBus::setNibble1(uint8_t skipUnEven) {
  uint8_t result = 0;
  if (skipUnEven) {
    if (servo[1].previousCurve & DIRECTION) result = 0b00001010;
    else result = 0b00000101;
  }
  else {
    /*
    if (servo3.previousCurve & DIRECTION) result |= (0x01 << 1);  // set bit 1;
    else result (0x01 << 0);                                      // set bit 0;
    if (servo4.previousCurve & DIRECTION) result |= (0x01 << 3);  // set bit 1;
    else result (0x01 << 2);                                      // set bit 0;
    */
  };
  return result;
}

//*****************************************************************************************************
//
// File:      myRSBus.h
// Author:    Aiko Pras
// History:   2025/05/05
//            2025/06/01 ap: first production version 
// 
// Purpose:   Declaration of RS-Bus feedback functions
//
//*****************************************************************************************************
#pragma once
#include <Arduino.h>                         // For general definitions
#include <RSBus.h>                           // Inherits and extends the RSBus class

class MyRsBus: public RSbusConnection {
  public:
    uint8_t feedbackNibble0;                 // The low nibble
    uint8_t feedbackNibble1;                 // The high nibble 
    uint8_t feedback8Bit;                    // The low and high nibbl;e together 

    void init(uint8_t address, bool skip);
    void checkRSFeedback();                  // Should be called from main as frequent as possible
    
    void sendNibble0(uint8_t position);
    void sendNibble1(uint8_t position);

    void sendFB01(uint8_t position);
    void sendFB23(uint8_t position);
    void sendFB45(uint8_t position);
    void sendFB67(uint8_t position);

    uint8_t setNibble0(uint8_t skipUnEven);  // Determines the value for the first feedback nibble
    uint8_t setNibble1(uint8_t skipUnEven);  // Determines the value for the second feedback nibble
};

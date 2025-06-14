// *****************************************************************************************************
//
// File:      configure.cpp
// Author:    Aiko Pras
// History:   2025/05/23
//            2025/06/01 ap: first production version 
// 
// Purpose:   Via a normal handheld, configuration of the servo speed and Tresholds
//
// Configuration of servo speed and Tresholds can be performed via a normal DCC handheld.
// For that purpose, the decoder listens to the local address 7000 + RS-Bus address - 1.
// Example: if the switch address on a Lenz LH100 is 397..400, the RS-Bus address will become 100
// and the loco address 7099. This loco address can also be used for configuration of CVs via PoM.
//
// To enter configuration mode, push F9 multiple times on and off, within a certain time interval.
// The number of times is determined by the ATTEMPTS_REQUIRED define. The interval in which these 
// pushes are needed, is given by the ATTEMPTS_INTERVAL define. Time is in milliseconds.
//
// - F9: enter / leave configuration mode
// - F1 .. F4: select servo 1 ..4
// - F5: Set servo in middle position, as well as both tresholds (1500 us). 
// - F6: Set sevo speed (default = 6)
// - F7: Set treshold 1 (straight)
// - F8: Set treshold 2 (diverging)
// - F0: store current setting in EEPROM (while in F6, F7 or F8)
//
// *****************************************************************************************************
#pragma once
#include <Arduino.h>                        // For general definitions

#define F9_ATTEMPTS_REQUIRED    2           // # F9 ON/OFF pushes before configuration mode is entered
#define F9_ATTEMPTS_INTERVAL 3000           // Number of milliseconds for all F9 pushes

#define NUMBER_OF_SERVO_MOVES   2           // Number of times a servo should move for identification
#define CONFIG_STEP_TIME      200           // Time between set treshold movements (im ms)


class Configure {
  
  public:
    bool checkStart(uint8_t F9F12);         // Is F9 pushed ON/OFF the number of ATTEMPTS_REQUIRED?
    bool checkConfig();                     // Should be called from the main loop as often as possible

    void setF0F4(uint8_t F0F4);             // Saves the value of F0 .. F4 
    void setF5F8(uint8_t F5F8);             // Saves the value of F5 .. F8 
    void setF9F12(uint8_t F9F12);           // Saves the value of F9 .. F12 
    void setSpeed(uint8_t sp, bool dir);    // Handles the loco speed command


  private:
    typedef enum {                          // Which configuration types are possible?
      Waiting,                              // Configuration mode has not started yet
      Ready,                                // Configuration mode started
      Verify,                               // To verify if we elected the right servo
      Set,                                  // We can set now
      Middle,                               // Servo middle Treshold (1500 us)
      TresholdStraight,                     // For the straight position (green / + / 1)
      TresholdDiverging,                    // For the diverging position (red / - /0)
      Speed                                 // Peeds of the servo movement
    } State_t;
    State_t configState = Waiting;

    uint8_t F0F4 = 0;                       // Local copy from the locoCmd object
    uint8_t F5F8 = 0;                       // Local copy
    uint8_t F9F12 = 0;                      // Local copy
    uint8_t locoSpeed = 0;                  // Local copy
    bool locoDirection = 0;                 // Local copy - True = Forward / False = Reverse

    uint8_t selectedServo = 255;            // which servo we are operating on??    

    // routines for each state
    bool doReady();
    void doVerify();
    void doSet();
    void doMiddle();
    void doTresholdStraight();
    void doTresholdDiverging();
    void doSpeed();

    uint8_t validServoFromF();              // One and only 1 servo related function is selected
    bool servoDeselected();                 // the previous selected servo is no longer selected

    // For verify if we selected the right servo
    void moveServo(uint8_t servoNumber);
    uint8_t numberOfMoves = NUMBER_OF_SERVO_MOVES;

    // For entering configuration mode
    uint8_t F9_AttemptsLeft = F9_ATTEMPTS_REQUIRED;
    long F9_TimeFirstPush = 0;
    long lastConfigTime;
    
    uint16_t currentPulseWidth;             // Needed in doPositioning() while setting the tresholds
};

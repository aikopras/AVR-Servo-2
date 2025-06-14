// *****************************************************************************************************
//
// File:      AVR_2_Servos.ino
// Author:    Aiko Pras
// History:   2022/07/12 AP Version 2.0
//            2025/02/16 AP Version 2.1
//            2025/06/01 AP Version 2.2 - First production version
//
// Purpose:   DCC Servo decoder for the first AVR Servo board, with two servo connections
//
// Processor:   AVR64DA28
// Programmer:  Serial UPDI  (see hardware.h for compile settings)
// Decoder PCB: Servo Decoder V2.0  2022/7
//
// ******************************************************************************************************
#include <Arduino.h>              // For general definitions
#include <AP_DCC_Decoder_Core.h>  // To include all objects, such as dcc, accCmd, etc.
#include "hardware.h"             // The pin and EEPROM specific details
#include "servo_CVs.h"            // Servo specific CVs
#include "servo_position.h"       // Storage for the servo positions in EEPROM
#include "myServo.h"              // Inherits and extends the ServoMoba class
#include "myRSBus.h"              // Perfroms all RS-Bus feedback functions
#include "configure.h"            // Allows configuration via the hand held

#define SKETCH_VERSION 2.2


MyServo servo[NUMBER_OF_SERVOS];  // MyServo is a new class, which inherits ServoMoba (and Servo)
MyRsBus rsbus;                    // MyRsBus inherits RSbusConnection, and is used for feedback bits
BasicLed configLed;               // Instantiate the extra green LED to show configuration mode


ToggleButton buttonPos0;
ToggleButton buttonPos1;


Configure handheldConfig;         // object that takes care of configuration via the hand held
bool configMode = false;          // Flag that tells if we are (not) in hand held configuration mode
bool skipUnEven;                  // If true, decoder uses even adresses only


//******************************************************************************************************
void setup() {
  // Step 1: Serial monitor.
  // Identify the decoder
  delay(100);
  Monitor.begin(115200);
  delay(100);
  Monitor.println();
  Monitor.println("Servo2Decoder");
  Monitor.print("Version: "); Monitor.println(SKETCH_VERSION);

  // Step 2: If the EEPROM has not yet been initialised, initialise the SERVO SPECIFIC values.
  // This involves all EEPROM values from index 65 and higher, and includes three parts:
  // 1) servo specific CVs (65..128/192),
  // 2) the (2 or 4) curves that are stored in EEPROM
  // 3) the circular buffer, which holds the last positions
  if (cvValues.notInitialised()) CreateDefaultServoValuesInEEPROM();
  // CreateDefaultServoValuesInEEPROM();     // May be used to temporarily reinitialise the EEPROM
  //
  // Step 3: Set the default CV values (1..64; see AP_CV_values.h. for details)
  // Decoder type (DecType) and software version (version) are set using cvValues.init().
  // After cvValues.init() is called, CV default values may be modified using cvValues.defaults[...]
  // Updated values take effect after setDefaults() is called, either by a long push of the
  // onboard programming button, or by a CV-POM or CV-SM message to CV8 (VID).
  cvValues.init(ServoDecoder, 20);
  //
  // Step 4: initialise the dcc and rsbus objects with addresses and other settings
  // This includes attaching the pins of the DCC input, the RS-Bus in- and output,
  // the onboard LED and button, and the Accessory Address as stored in the CV.
  // In this step we also initialise the CV values (1..64), if the EEPROM wasn't initialised yet
  decoderHardware.init();
  //
  // Step 4: Read the skipUnEven CV
  // Skipping uneven (or even) addresses avoids potential problems with RS-Bus switch feedback 
  // See: https://github.com/aikopras/RSbus/blob/master/extras/switch-feedback-problems.md
  skipUnEven = cvValues.read(skipUnEven);
  //
  // Step 5: Attach the extra (green) LED, which we will use to signal we are in configuration mode
  configLed.attach(LED_CONFIG);
  //
  // Step 6: Initialse the servos
  // The main part of this sketch is responsible for the servo initialisation and movement
  // See myServo for details
  servo[0].init(0);
  servo[1].init(1);
  //
  // Step 7: Initialise the object for RS-Bus feedback messages. The address is taken from
  // the myRSAddr CV. We need skipUnEven to determine of each servo has its own feedback nibble,
  // or if two servos share the same RS-Bus nibble.
  rsbus.init(cvValues.read(myRSAddr), skipUnEven);
  //
  // Step 8: Connect the two buttons that can be used to change the servo's position
  buttonPos0.attach(POSITION0_PIN, DEBOUNCE_TIME);
  buttonPos1.attach(POSITION1_PIN, DEBOUNCE_TIME);
  //
  printAddresses();
}


//******************************************************************************************************
//
//******************************************************************************************************
void loop() {
  // Step1: Check if we are in normal mode or configuration mode
  if (!configMode) {    // We are in normal mode for servo operation
    if (dcc.input()) {  // Any DCC command received?
      switch (dcc.cmdType) {
        case Dcc::MyAccessoryCmd:
          onBoardLed.activity();
          // printAccessoryDetails();  // for debugging
          // If skipUnEven is true, turnout 1 and turnout 2 will be used for servo[0]
          // whereas turnout 3 and turnout 4 are for servo[1]
          // If skipUnEven is false, turnout 1 is for servo[0] and turnout 2 for servo[1]
          // In that case, turnout 3 and 4 will not be used.
          if ((accCmd.activate) && skipUnEven) {
            switch (accCmd.turnout) {
              case 1:
                servo[0].set(accCmd.position);
                rsbus.sendNibble0(accCmd.position);
                break;
              case 2:
                servo[0].set(accCmd.position);
                rsbus.sendNibble0(accCmd.position);
                break;
              case 3:
                servo[1].set(accCmd.position);
                rsbus.sendNibble1(accCmd.position);
                break;
              case 4:
                servo[1].set(accCmd.position);
                rsbus.sendNibble1(accCmd.position);
                break;
            };
          } else {  // skipUnEven = false
            switch (accCmd.turnout) {
              case 1:
                servo[0].set(accCmd.position);
                rsbus.sendFB01(accCmd.position);
                rsbus.send4bits(LowBits, rsbus.feedbackNibble0);
                break;
              case 2:
                servo[1].set(accCmd.position);
                rsbus.sendFB23(accCmd.position);
                rsbus.send4bits(LowBits, rsbus.feedbackNibble0);
                break;
              case 3: break;  // Might be used for servo-3 => sendFB45(position)
              case 4: break;  // Might be used for servo-4 => sendFB67(position)
            };
          }
          break;  // Dcc::MyAccessoryCmd

        case Dcc::MyPomCmd:
          // Note: I have a problem in my Programmer Decoder PoM: My maximum CV number is 8 (instead of 10) bits
          cvProgramming.processMessage(Dcc::MyPomCmd);
          Monitor.print("PoM Command. ");
          Monitor.print("Received CV Number: ");
          Monitor.print(cvCmd.number);
          //Monitor.print(" - Received CV Value: ");
          //Monitor.print(cvCmd.value);
          Monitor.println();
          break;

        case Dcc::SmCmd:
          cvProgramming.processMessage(Dcc::SmCmd);
          break;

        case Dcc::MyLocoF9F12Cmd:
          // Check if F9 is pushed and we need to switch to configuration mode
          configMode = handheldConfig.checkStart(locoCmd.F9F12);
          break;

        default:
          // Nothing
          break;
      };                           // end switch
    };                             // end of DCC input
  }                                // end of normal mode
  else {                           // This is the mode to configure servo speed and position
    if (dcc.input()) {             // Any DCC command received?
      switch (dcc.cmdType) {
        case Dcc::MyLocoSpeedCmd: handheldConfig.setSpeed(locoCmd.speed, locoCmd.forward); break;
        case Dcc::MyLocoF0F4Cmd:  handheldConfig.setF0F4(locoCmd.F0F4);   break;
        case Dcc::MyLocoF5F8Cmd:  handheldConfig.setF5F8(locoCmd.F5F8);   break;
        case Dcc::MyLocoF9F12Cmd: handheldConfig.setF9F12(locoCmd.F9F12); break;
        default: break;  // Nothing
      };
    configMode = handheldConfig.checkConfig();  // Should be called as frequent as possible
    };    // end of DCC input
  };      // end of config mode
  //
  // Step 2: as frequent as possible update the RS-Bus hardware, check if the programming
  // button is pushed, and if the status of the onboard LED should be changed.
  decoderHardware.update();
  //
  // Step 3: as frequent as possible check if a RSBus feedback message should be send.
  rsbus.checkRSFeedback();
  //
  // Step 4: as frequent as possible check if one or more servos require updates
  servo[0].checkServo();
  servo[1].checkServo();
  //
  // Step 5: Check the buttons if switch positions should be changed
  // This is implemented on board V2.0 (2022/07), but will be removed on futire boards
  buttonPos0.read();
  if (buttonPos0.changed()) {
    bool newPos = !servo[0].getPosition();
    servo[0].set(newPos);
    if (skipUnEven) rsbus.sendNibble0(newPos);
    else {
      rsbus.sendFB01(newPos);
      rsbus.send4bits(LowBits, rsbus.feedbackNibble0);      
    }
  };
  buttonPos1.read();
  if (buttonPos1.changed()) {
    bool newPos = !servo[1].getPosition();
    servo[1].set(newPos);
    if (skipUnEven) rsbus.sendNibble1(newPos);
    else {
      rsbus.sendFB23(newPos);
      rsbus.send4bits(LowBits, rsbus.feedbackNibble0);  
    }  
  };
  // 
};


//******************************************************************************************************
// Some temporary print routines for debugging
//******************************************************************************************************
void printCVs() {
  for (uint8_t i = 0; i <= 64; i++) {
    Monitor.print("CV");
    Monitor.print(i);
    Monitor.print(": ");
    Monitor.println(cvValues.read(i));
  }
  if (cvValues.addressNotSet()) Monitor.println("Address not set");
  Monitor.print("Decoder Address: ");
  Monitor.println(cvValues.storedAddress());
  Monitor.print("RS-Bus Feedback Address: ");
  Monitor.println(rsbus.address);
}


void printAccessoryDetails() {
  Monitor.println("");
  Monitor.print("Switch address: ");
  Monitor.print(accCmd.outputAddress);
  Monitor.print(", Position: ");
  if (accCmd.position == 1) Monitor.println("+ (1)");
  else Monitor.println("- (0)");
};

void printAddresses() {
  Monitor.println("");
  Monitor.print("Decoder adres: ");
  Monitor.println(cvValues.storedAddress());
  Monitor.print("RS-Bus adres: ");
  Monitor.println(cvValues.read(myRSAddr));
  Monitor.print("Loco adres: ");
  Monitor.println(cvValues.storedAddress() + 7000);
  Monitor.println("");
}

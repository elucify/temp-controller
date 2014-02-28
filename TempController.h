#ifndef __TEMPCONTROLLER_H_
#define __TEMPCONTROLLER_H_

#include <Arduino.h>
#include "LedControl.h"

// Now we need a LedControl to work with.
// ***** These pin numbers will probably not work with your hardware *****
// pin 13 is connected to the DataIn 
// pin 11 is connected to the CLK 
// pin 12 is connected to LOAD 
// We have only a single MAX72XX.
 
#define LC_DIN (13)
#define LC_CLK (11)
#define LC_LOAD (12)
#define LC_BRIGHTNESS (8)

//
// Alert LEDs
//
#define LED_A_BLUE  2
#define LED_A_RED   8

#define LED_B_BLUE 64
#define LED_B_RED  128

//
// Channel constants
//
#define CHANNEL_A 0
#define CHANNEL_B 1

//
// Digits
//
#define DIGIT1 (7)
#define DIGIT2 (6)
#define DIGIT3 (5)
#define DIGIT4 (4)
#define DIGIT_LEDS (3)

//
// Buttons (number is Arduino pin)
// BUTTON_ANY means operate on both
//
#define BUTTON_UP (8)
#define BUTTON_DOWN (9)
#define BUTTON_ANY (0xff)

// Two pins for switch right and left.
// Center means both high (buttons/switch are active low)
#define SWITCH_RIGHT (10)
#define SWITCH_LEFT (7)
#define SWITCH_CENTER (0xff)

//
// Magic values set digits to specialized states
//
#define CHANNEL_CLEAR ((byte)(0xff))
#define CHANNEL_DASHES ((byte)(0xfe))

//
// DisplayBoard
// Controls display and dispatches messages
//
class DisplayBoard {

public:
    //DisplayBoard();
   DisplayBoard(byte pinDin = LC_DIN, byte pinClk = LC_CLK, byte pinLoad = LC_LOAD,
                byte brightness = LC_BRIGHTNESS, byte numDevices = 1);
   virtual ~DisplayBoard();

   // Individual digit, led methods
   void digit(byte digit, char value, boolean dp = false);
   void bits(byte digit, byte value);
   void led(byte which, boolean value);
   void leds(byte value);

   // Channel A, B methods
   void temp(byte channel, byte value);
   void temp(byte channe, float value);
   void hot(byte channel, boolean value);
   void cold(byte channel, boolean value);

   // Button/switch methods
   boolean isButtonPushed(byte which = BUTTON_ANY);

   boolean isSwitchLeft();
   boolean isSwitchRight();
   boolean isSwitchCenter();
   byte switchPosition();

protected:
   LedControl _lc;
   static const byte _ledlist[];
   byte _leds_bits;

   void init(byte pinDin, byte pinClk, byte pinLoad,
             byte brightness, byte numDevices);
};

// Doesn't yet do anything except initialize DisplayBoard
// Probably DisplayBoard should be in a separate library
class TempController {
public:
    TempController(byte pinDin = LC_DIN, byte pinClk = LC_CLK, byte pinLoad = LC_LOAD,
                   byte brightness = LC_BRIGHTNESS, byte numDevices = 1);
    void init();
    virtual ~TempController();

protected:
    DisplayBoard _db;
};

#endif


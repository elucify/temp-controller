#include "Arduino.h"
#include "TempController.h"

//
// Notes: Refactoring test program to use TempController object
// Not yet instantiating TempController in program
// Need to design and implement display board API
// Then refactor main program to use this API instead of lc.
//

const byte DisplayBoard::_ledlist[] = { LED_A_BLUE, LED_A_RED, LED_B_BLUE, LED_B_RED };

//DisplayBoard::DisplayBoard() {
//    init(LC_DIN, LC_CLK, LC_LOAD, LC_BRIGHTNESS, 1);
//}

DisplayBoard::DisplayBoard(byte pinDin, byte pinClk, byte pinLoad, byte brightness, byte numDevices):
    _lc(pinDin, pinClk, pinLoad, numDevices)
{
  // Initialize display chip
  init(pinDin, pinClk, pinLoad, brightness, numDevices);

  _leds_bits = 0;
}

void DisplayBoard::init(byte pinDin, byte pinClk, byte pinLoad, byte brightness, byte numDevices)
{
    // numDevices ignored--controller only handles 1 device
    // Wake up MAX72XX -- it's in powersave mode on startup
    _lc.shutdown(0, false);
    
    // Set brightness
    _lc.setIntensity(0, brightness);

    // Clear display
    _lc.clearDisplay(0);
    _leds_bits = 0;

    // intialize inputs
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(SWITCH_LEFT, INPUT_PULLUP);
    pinMode(SWITCH_RIGHT, INPUT_PULLUP);
}

// Buttons are active low
boolean DisplayBoard::isButtonPushed(byte which) {
    if (which == BUTTON_ANY) {
        return isButtonPushed(BUTTON_UP) || isButtonPushed(BUTTON_DOWN);
    }
    return !digitalRead(which);
}

boolean DisplayBoard::isSwitchLeft() {
    return isButtonPushed(SWITCH_LEFT);
}

boolean DisplayBoard::isSwitchRight() {
    return isButtonPushed(SWITCH_RIGHT);
}

boolean DisplayBoard::isSwitchCenter() {
    return !(isButtonPushed(SWITCH_RIGHT) || isButtonPushed(SWITCH_LEFT));
}

byte DisplayBoard::switchPosition() 
{
    return isButtonPushed(SWITCH_CENTER) ? SWITCH_CENTER :
        isButtonPushed(SWITCH_RIGHT) ? SWITCH_RIGHT : SWITCH_LEFT;
}

void DisplayBoard::digit(byte digit, char value, boolean dp)
{
    _lc.setChar(0, digit, value, dp);
}

void DisplayBoard::bits(byte digit, byte value)
{
    _lc.setRow(0, digit, value);
}

/* Set leds using an integer 0..15 */
void DisplayBoard::leds(byte value)
{
    for (int i = 0; i <= 3; i++) {
        led(_ledlist[3-i], value & (1 << i));
    }
}

// Set leds using LED_X_COLOR constants
void DisplayBoard::led(byte which, boolean value)
{
    if (value) {
        _leds_bits |= which;
    } else {
        _leds_bits &= ~which;
    }
    _lc.setRow(0, DIGIT_LEDS, _leds_bits);
}

// Set temperature for channel
void DisplayBoard::temp(byte channel, byte value)
{
    byte dp = 0;
    char top_digit = ' ', bottom_digit = ' ';

    switch (value) {
    case CHANNEL_CLEAR: top_digit = bottom_digit = ' '; dp = 0; break;
    case CHANNEL_DASHES: top_digit = bottom_digit = '-'; dp = 0; break;
    default:

        // >100F indicated by ones digit dp set, >200F by tens digit dp set
        dp = ((value >= 200) ? 2 : 0) | ((value >= 100) ? 1 : 0);

        value %= 100;

        top_digit = '0' + (value / 10);
        bottom_digit = '0' + (value % 10);

        break;
    }

    value %= 100;

    if (channel == CHANNEL_A) {
        digit(DIGIT1, value / 10, dp & 2);
        digit(DIGIT2, value % 10, dp & 1);
    } else if (channel == CHANNEL_B) {
        digit(DIGIT3, value / 10, dp & 2);
        digit(DIGIT4, value % 10, dp & 1);
    }
}

// Handle float temperatures
void DisplayBoard::temp(byte channel, float value)
{
    temp(channel, (byte)value);
}

void DisplayBoard::hot(byte channel, boolean value)
{
    led((channel == CHANNEL_A) ? LED_A_RED : LED_B_RED, value);
}

void DisplayBoard::cold(byte channel, boolean value)
{
    led((channel == CHANNEL_A) ? LED_A_BLUE : LED_B_BLUE, value);
}

DisplayBoard::~DisplayBoard()
{
}

//
// TempController
//

TempController::TempController(byte pinDin, byte pinClk, byte pinLoad, byte brightness, byte numDevices):
  _db(pinDin, pinClk, pinLoad, brightness, numDevices)
{
    init();
}

void TempController::init()
{
}

TempController::~TempController()
{
}


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
  _displayBoard(pinDin, pinClk, pinLoad, brightness, numDevices)
{
    init();
}

void TempController::run()
{
    float a = _thermometers[CHANNEL_A]->getTemperature();
    float b = _thermometers[CHANNEL_B]->getTemperature();

    if (a > -100.0) {
        temp_a = a;
    } else {
        Serial.println("A failed");
    }
    if (b > -100.0) {
        temp_b = b;
    } else {
        Serial.println("b failed");
    }
  
    float diff_a = temp_a - ref_temp_a;
    float diff_b = temp_b - ref_temp_b;

    Serial.println("");
    Serial.print("a:");
    Serial.print(temp_a);
    Serial.print(":");
    Serial.print(diff_a);
    Serial.print(":b:");
    Serial.print(temp_b);
    Serial.print(":");
    Serial.println(diff_b);
  
    updateDisplay(temp_a, temp_b,
                  diff_a < (-1 * tolerance_a),
                  diff_a > tolerance_a,
                  diff_b < (-1 * tolerance_b),
                  diff_b > tolerance_b);
  
    delay(2000);
}

// Read thermometers and write to ROM -or- read thermometer addresses from ROM
void TempController::initializeThermometers()
{
    // TODO: READ ROM
    DeviceAddress thermometer_address_a = { 0x28, 0xFC, 0x74, 0xC9, 0x02, 0x00, 0x00, 0x86 };
    DeviceAddress thermometer_address_b = { 0x28, 0xDA, 0x25, 0x55, 0x05, 0x00, 0x00, 0x53};
    

    // Copy addresses to working memory
    memcpy(_thermometer_address[CHANNEL_A], thermometer_address_a, 8);
    memcpy(_thermometer_address[CHANNEL_B], thermometer_address_b, 8);

    // Initialize thermometer objects
    _thermometers[CHANNEL_A] = new Thermometer(_thermometer_address[CHANNEL_A]);
    _thermometers[CHANNEL_B] = new Thermometer(_thermometer_address[CHANNEL_B]);

    OneWire *wire = Thermometer::getOneWire();


    // If both buttons down on power-up, read and
    // report sensors
    if (_displayBoard.isButtonPushed(BUTTON_UP) &&
        _displayBoard.isButtonPushed(BUTTON_DOWN)) {

        byte addr[8];
        byte naddrs = 0;

        Serial.println("> Scanning devices");

        while(wire->search(addr)) {

            naddrs++;

            Serial.print("Device ");
            Serial.println(naddrs);

            if (OneWire::crc8(addr, 7) != addr[7]) {
                Serial.print("INVALID: ");
            }

            for (byte i = 0; i < 8; i++) {
                if (!(addr[i] & 0xf0)) {
                    Serial.print("0");
                }
                Serial.print(addr[i], HEX);
                Serial.print(i == 7 ? "\n" : " ");
            }
        }
        wire->reset_search();

        Serial.print("> Devices found: ");
        Serial.println(naddrs);
    }
}

// Initialize thermometers
void TempController::init()
{
    initializeThermometers();

    // Initialize internal states
    state = STATE_WAIT_BUTTON_DOWN;
    prevstate = 0xff;
    count = 0;
    holdcount = 1000 / DELAYTIME; // After 2 seconds, start counting fast
    holdperiod = HOLDPERIOD0;
    temp_a = temp_b = 0.0;
    tolerance_a = tolerance_b = 0.5;
    ref_temp_a = ref_temp_b = 48.0;

    updateDisplay(temp_a, temp_b, false, false, false, false);    
}

void TempController::updateDisplay(float a, float b,
   boolean cold_a = false,
   boolean hot_a = false,
   boolean cold_b = false,
   boolean hot_b = false)
{
  _displayBoard.temp(CHANNEL_A, a);
  _displayBoard.temp(CHANNEL_B, b);
  
  _displayBoard.hot(CHANNEL_A, hot_a);
  _displayBoard.hot(CHANNEL_B, hot_b);

  _displayBoard.cold(CHANNEL_A, cold_a);
  _displayBoard.cold(CHANNEL_B, cold_b);
}

void TempController::printButtons()
{
    Serial.print("UP = ");
    Serial.println(_displayBoard.isButtonPushed(BUTTON_UP));
    Serial.print("DOWN = ");
    Serial.println(_displayBoard.isButtonPushed(BUTTON_DOWN));
    Serial.print("RIGHT = ");
    
    Serial.print("Switch is ");
    if (_displayBoard.isSwitchRight()) {
      Serial.println("RIGHT");
    }
    if (_displayBoard.isSwitchLeft()) {
      Serial.println("LEFT");
    }
    
    if (_displayBoard.isSwitchCenter()) {
      Serial.println("CENTER");
    }
}

TempController::~TempController()
{
}


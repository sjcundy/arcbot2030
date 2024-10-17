/*
  XMAS Project

  Christmas project for work gift exchange. Simply circuit and logic
  to display Temperature, Humidity, and a Sit/Stand Timer using
  both LEDs and OLED

  Component Checklist:
  1x Arduino Uno
  1x DHT22
  1x SSD1306 OLED Display
  2x 6mm Tactile Button Swtich
  1x SW-520D Tilt Sensor
  8x LED: 2xRed,2xGreen,2xBlue,2xWhite
  9x Resistors: 8x220Ohm, 1x10kOhm

  The Circuit:
  DHT22
    VCC     -> Arduino:5V
    DATA     -> Arduino:D2
    DATA     -> 10kOhm -> Arduino:5V
    NC      -> N/A
    GND     -> Arduino:GND
  SSD1306
    GND     -> Arduino:GND
    VCC     -> Arduino:5V
    SDA     -> Arduino:A4
    SCL     -> Arduino:A5
  SW-520D
    T1      -> Arduino:GND
    T2      -> Arduino:3
  Button (Power)
    T1A     -> Arduino:4 (INPUT_PULLUP)
    T2A     -> Arduino:GND
  Button (Mode)
    T1A     -> Arduino:5 (INPUT_PULLUP)
    T2A     -> Arduino:GND
  LED_WHITE_1
    Anode   -> Arduino:6
    Cathode -> 220Ohm -> Arduino:GND
  LED_WHITE_2
    Anode   -> Arduino:7
    Cathode -> 220Ohm -> Arduino:GND
  LED_RED_1
    Anode   -> Arduino:8
    Cathode -> 220Ohm -> Arduino:GND
  LED_RED_2
    Anode   -> Arduino:9
    Cathode -> 220Ohm -> Arduino:GND
  LED_GREEN_1
    Anode   -> Arduino:10
    Cathode -> 220Ohm -> Arduino:GND
  LED_GREEN_2
    Anode   -> Arduino:11
    Cathode -> 220Ohm -> Arduino:GND
  LED_BLUE_1
    Anode   -> Arduino:12
    Cathode -> 220Ohm -> Arduino:GND
  LED_BLUE_2
    Anode   -> Arduino:13
    Cathode -> 220Ohm -> Arduino:GND

  Created 09.29.2024
  By Steve Cundy
  Modified 09.29.2024
  By Steve Cundy

  https://github.com/sjcundy/arcbot2030
  https://wokwi.com/projects/409850507112597505

*///###################################################################
// Include the necessary Libraries
//###################################################################
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//###################################################################
// Define Macros
//###################################################################
// Standard Debug Macros
//#define DEBUG_ENABLE //Comment out to disable Serial.print debugging
#ifdef DEBUG_ENABLE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
//Define OLED Display Dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
//###################################################################
// Define Enumeration Constants
//###################################################################
// Modes Available
enum Modes {
  MODE_TEMP = 0,
  MODE_HUMIDITY,
  MODE_TIMER,
  MODE_CYCLE,
  NUM_MODES
};

// Delays for Debounce, Hardware, Timer, etc
enum Delays {
  DELAY_TMPHUM = 0,
  DELAY_TILT,
  DELAY_TIMER,
  DELAY_CYCLE,
  NUM_DELAYS
};

// Types of LEDs
enum Leds {
  LED_WHITE1 = 0,
  LED_WHITE2,
  LED_RED1,
  LED_RED2,
  LED_GREEN1,
  LED_GREEN2,
  LED_BLUE1,
  LED_BLUE2,
  NUM_LEDS
};

// Type os Buttons
enum Buttons {
  BUTTON_POWER = 0,
  BUTTON_MODE,
  NUM_BUTTONS
};

// OLDED Messages
enum Messages {
  MSG_WELCOME = 0,
  MSG_GOODBYE,
  MSG_TEMP,
  MSG_HUMIDITY,
  MSG_TIMER,
  NUM_MSG
};
//###################################################################
// Define Pins
//###################################################################
const uint8_t dht22SensorPin = 2;
const uint8_t tiltSensorPin = 3;
const uint8_t buttonPin[NUM_BUTTONS] = { 4, 5 };
const uint8_t ledPin[NUM_LEDS] = { 6, 7, 8, 9, 10, 11, 12, 13 };

//###################################################################
// DHT22 Initialization and definitions
//###################################################################
// Initialize the DHT22
DHT dht(dht22SensorPin, DHT22);

//###################################################################
// OLED Initialization and definitions
//###################################################################
// Initialize the OLED
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define OLED Messages (see enum Messages for order)
const char* oledMessage[NUM_MSG] = { 
  "ARCBOT2030", 
  "(-_-)",
  "%dC / %dF",
  "%d%%",
  "%d min"
};
const char* oledModes[NUM_MODES] = {
  "TEMPERATURE",
  "HUMIDITY",
  "STAND TIME",
  "CYCLE"
};

// Scrolling Variables
bool scrolling = false;
uint16_t scrollPosition = 0;
unsigned long previousScrollMillis = 0;
const uint16_t scrollInterval = 300; // Scroll every 300 ms
const char* currentScrollingMessage = nullptr;
uint16_t messageLength = 0;
// Previous display buffers to track changes
static char previousLine1[17] = {0}; // Previous content of line 1
static char previousLine2[17] = {0}; // Previous content of line 2

//###################################################################
//Define Delays for Hardware, Timers, etc (se enum Buttons and Delays for order)
//###################################################################
const unsigned long debounceDelay[NUM_BUTTONS] = { 50, 50 };
const unsigned long hardwareDelay[NUM_DELAYS] = { 5000, 500, 60000, 10000 };
unsigned long lastDelayTime[NUM_DELAYS] = { 0 };

//###################################################################
//System Variables
//###################################################################
const uint8_t MODE_LED_COUNT = 2;
unsigned long timeToStand = 0;
bool isButtonPressed = false;
bool isStartup = false;
bool isShutdown = false;

//###################################################################
//User Defined Variables
//###################################################################
bool isPowerOn = true;             // Default: true; decide if you want display/led on or off when plugged in after startup animation
bool debugEnable = true;           // Default: false; used for serial debugging
int ledBlinkDelay = 250;           // Default: 500; pause between LEDs blinking for animations
uint8_t ledFlashNum = 3;           // Default: 3; how many times LEDs should flash in startup/shutdown animation
uint8_t modeSelected = MODE_CYCLE;  // Default: MODE_CYCLE; MODE_TEMP, MODE_HUMIDITY, MODE_TIMER, MODE_CYCLE
int maxSitMinutes = 30;             // Default: 30; how many minutes should the sit/stand timer count for

//###################################################################
//Function prototype
//###################################################################
bool debounceButton(uint8_t buttonIndex);
void runMode();
bool ledFlashAll(int numFlash, int ledDelay = 0);
bool ledAnimation(int startIndex = 0, bool ledState = LOW, int ledDelay = 0);
void toggleLeds(uint8_t startIndex = 0, bool ledState = LOW);
void displayMessage(const char* message);
void chkPowerButton();
void chkModeButton();
void chkTiltSensor();
void modeTemp();
void modeHumidity();
void modeTimer();
void modeCycle();

//###################################################################
// Setup Initialization Loop
//###################################################################
void setup() {
  #ifdef DEBUG_ENABLE
  Serial.begin(9600);
  #endif
  // Initialize DHT22
  dht.begin();

  // Initialize the OLED display
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Check the I2C address
    DEBUG_PRINTLN("SSD1306 allocation failed");
    for(;;);  // Infinite loop to stop execution if initialization fails
  }

  oled.clearDisplay();               // Clear the display

  // Set Pin Modes
  for (uint8_t i = 0; i < NUM_LEDS; i++) { pinMode(ledPin[i], OUTPUT); }
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) { pinMode(buttonPin[i], INPUT_PULLUP); }
  // pinMode(dht22SensorPin, INPUT_PULLUP);
  pinMode(tiltSensorPin, INPUT_PULLUP);

  // Initialize Variables
  isStartup = true;
  timeToStand = millis() + ((unsigned long)maxSitMinutes * 60000UL);

  // Startup Functions
  
  // Initialze Sensors
}
//###################################################################
// Main Program Loop
//###################################################################
void loop() {
  if (isStartup) {
    if (ledAnimation(0, HIGH, ledBlinkDelay)) { 
      isStartup = false; 
    }
  } else if (isShutdown) {
    if (ledAnimation(0, LOW, ledBlinkDelay)) { 
      isShutdown = false;
    }
  } else {
    chkPowerButton();
    if (isPowerOn) {
      chkTiltSensor();
      chkModeButton();
      runMode();
    }
    isButtonPressed = false;  // Ensure that if Button was pressed it is now reset
  }
}

//###################################################################
// System Defined Functions
//###################################################################
bool debounceButton(uint8_t buttonIndex) {
  static uint8_t buttonState[NUM_BUTTONS] = { HIGH, HIGH };
  static uint8_t lastReading[NUM_BUTTONS] = { HIGH, HIGH };
  static unsigned long lastDebounceTime[NUM_BUTTONS] = { 0, 0 };

  uint8_t reading = digitalRead(buttonPin[buttonIndex]);

  if (reading != lastReading[buttonIndex]) {
    lastDebounceTime[buttonIndex] = millis();
  }
  if ((millis() - lastDebounceTime[buttonIndex]) > debounceDelay[buttonIndex]) {
    if (reading != buttonState[buttonIndex]) {
      buttonState[buttonIndex] = reading;
      if (buttonState[buttonIndex] == LOW) {
        return true;
      }
    }
  }
  lastReading[buttonIndex] = reading;
  return false;
}

void runMode() {
  // Determine which mode to run
  switch (modeSelected) {
    case MODE_TEMP:
      modeTemp();
      break;
    case MODE_HUMIDITY:
      modeHumidity();
      break;
    case MODE_TIMER:
      modeTimer();
      break;
    case MODE_CYCLE:
      modeCycle();
      break;
  }
}
bool ledFlashAll(int numFlash, int ledDelay) {
  // LED Flash Animation
  static uint8_t ledState = LOW;
  static uint8_t ledFlashCount = 0;
  static unsigned long lastUpdateTime = 0;
  unsigned long currentMillis = millis();
  uint8_t totalFlash = (numFlash * 2) + 1;
  if (currentMillis - lastUpdateTime >= ledDelay) {
    lastUpdateTime = currentMillis;
    if (ledFlashCount == 0) {
      ledState = LOW;
      for (uint8_t i = 0; i < NUM_LEDS; i++) { digitalWrite(ledPin[i], ledState); }
    }
    if (ledFlashCount < totalFlash) {
      for (uint8_t i = 0; i < NUM_LEDS; i++) { digitalWrite(ledPin[i], ledState); }
      ledState = !ledState;
      ledFlashCount++;
    } else {
      ledFlashCount = 0;
      lastUpdateTime = 0;
      return true;
    }
  }
  return false;
}

bool ledAnimation(int startIndex, bool ledState, int ledDelay) {
  // Change state on selected LEDs with optional non-blocking delay
  static uint8_t ledIndex;
  static unsigned long lastUpdateTime;
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= ledDelay) {
    lastUpdateTime = currentMillis;
    if (ledIndex == 0) {
      if (ledState) {
        oled.ssd1306_command(SSD1306_DISPLAYON); 
        displayMessage(oledMessage[MSG_WELCOME]);
        isButtonPressed = true;
        }
      else {displayMessage(oledMessage[MSG_GOODBYE]);}
      for (uint8_t i = 0; i < NUM_LEDS; i++) { digitalWrite(ledPin[i], !ledState); }
    }
    if (ledIndex < NUM_LEDS) {
      digitalWrite(ledPin[ledIndex], ledState);
      ledIndex++;
    } else {
      if (ledFlashAll(ledFlashNum, ledBlinkDelay)) {
        if (!ledState) {
          oled.clearDisplay(); 
          oled.ssd1306_command(SSD1306_DISPLAYOFF);
        }
        ledIndex = startIndex;
        lastUpdateTime = 0;
        return true;
      }
    }
  }
  return false;
}

void toggleLeds(uint8_t startIndex, bool ledState) {
  // Toggle LED states sequentially
  for (uint8_t i = startIndex; i < NUM_LEDS; i++) {
    digitalWrite(ledPin[i], ledState);
  }
}

void displayMessage(const char* message) {
  // Display messages on OLED
  // 128wx64h: size 1=6wx8h and each size is xsize, size 2=12x16, size 3=18x24, etc
  // oled.setCursor(x, y);
  // oled.setTextSize(x);
  // oled.setTextColor(SSD1306_WHITE); // doesn't apply to the dual color displays
  // oled.print();
  // oled.println();
  // oled.clearDisplay();
  // oled.display();
  int textLength = strlen(message);
  int16_t x, y, x1, y1;
  uint16_t w, h;
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  if(isStartup) {
    oled.getTextBounds("BOOTING", 0, 0, &x1, &y1, &w, &h);
    x = (SCREEN_WIDTH - w) / 2;
    //y = (SCREEN_HEIGHT - h) / 2;
    oled.setCursor(x, 0);
    oled.println("BOOTING");
  }
  else if (isShutdown) {
    oled.getTextBounds("GOODBYE", 0, 0, &x1, &y1, &w, &h);
    x = (SCREEN_WIDTH - w) / 2;
    //y = (SCREEN_HEIGHT - h) / 2;
    oled.setCursor(x, 0);
    oled.println("GOODBYE");
  }
  else {
    oled.getTextBounds(oledModes[modeSelected], 0, 0, &x1, &y1, &w, &h);
    x = (SCREEN_WIDTH - w) / 2;
    //y = (SCREEN_HEIGHT - h) / 2;
    oled.setCursor(x, 0);
    oled.println(oledModes[modeSelected]);
  }
  oled.println();
  oled.setTextSize(2);
  oled.getTextBounds(message, 0, 24, &x1, &y1, &w, &h);
  x = (SCREEN_WIDTH - w ) / 2;
  //y = (SCREEN_WIDTH - h) / 2;
  oled.setCursor(x, 24);
  // oled.println("Test");
  oled.println(message);
  oled.display();
}

void chkPowerButton() {
  // Check to see if power button was pressed and based on previous state turn of/off display and LEDs
  if (debounceButton(BUTTON_POWER)) {
    isButtonPressed = true;
    isPowerOn = !isPowerOn;
    if (isPowerOn) {
      isStartup = true;
    } else {
      isShutdown = true;
    }
    DEBUG_PRINTLN("Power Button Pressed!");
  }
}

void chkModeButton() {
  // Check to see if mode button pressed and change accordingly in sequence defined by enum Modes
  if (debounceButton(BUTTON_MODE)) {
    isButtonPressed = true;
    modeSelected = (modeSelected + 1) % NUM_MODES;
    DEBUG_PRINT("Mode Button Pressed! Mode Selected: ");
    DEBUG_PRINTLN(modeSelected);
  }
}

void chkTiltSensor() {
  // Check to see if titl sensor was trigged to resent sit/stand timer
  static bool tiltState = LOW;
  if (lastDelayTime[DELAY_TILT] == 0 || millis() - lastDelayTime[DELAY_TILT] > hardwareDelay[DELAY_TILT]) {
    int newTiltState = digitalRead(tiltSensorPin);
    if (tiltState != newTiltState) {
      isButtonPressed = true;
      timeToStand = millis() + ((unsigned long)maxSitMinutes * 60000UL);
      DEBUG_PRINT("Tilt Detected! Time to Stand: ");
      DEBUG_PRINTLN(timeToStand);
      tiltState = newTiltState;
    }
    lastDelayTime[DELAY_TILT] = millis();
  }
}

void modeTemp() {
  static uint8_t oldValue = 0;
  static uint8_t tempC = 0;
  static uint8_t tempF = 0;
  static char buffer[32];
  bool isChanged = false;
  // Execute the temperature sensor reading and display based on values ensuring appropriate delay between readings
  if (isButtonPressed || lastDelayTime[DELAY_TMPHUM] == 0 || millis() - lastDelayTime[DELAY_TMPHUM] > hardwareDelay[DELAY_TMPHUM]) {
    tempC = round(dht.readTemperature());
    tempF = round(dht.readTemperature(true));
    if (isButtonPressed || oldValue != tempC) {
      oldValue = tempC;
      isChanged = true;
    }
    lastDelayTime[DELAY_TMPHUM] = millis();
    DEBUG_PRINTLN("Mode: Temperature: ");
    DEBUG_PRINT(tempC);
    DEBUG_PRINT("C, ");
    DEBUG_PRINTLN(tempF);
  }
  digitalWrite(ledPin[LED_WHITE1], HIGH);
  digitalWrite(ledPin[LED_WHITE2], LOW);
  // RangeLED:   2(>26), 23(24-26), 34(23-24), 45(21-23), 56(21-18), 67(16-18), 7(<16)
  toggleLeds(2, LOW);
  if (tempC > 26.0) {
    digitalWrite(ledPin[LED_RED1], HIGH);
  } else if (tempC > 24.0) {
    digitalWrite(ledPin[LED_RED1], HIGH);
    digitalWrite(ledPin[LED_RED2], HIGH);
  } else if (tempC > 23.0) {
    digitalWrite(ledPin[LED_RED2], HIGH);
    digitalWrite(ledPin[LED_GREEN1], HIGH);
  } else if (tempC > 21.0) {
    digitalWrite(ledPin[LED_GREEN1], HIGH);
    digitalWrite(ledPin[LED_GREEN2], HIGH);
  } else if (tempC > 18.0) {
    digitalWrite(ledPin[LED_GREEN2], HIGH);
    digitalWrite(ledPin[LED_BLUE1], HIGH);
  } else if (tempC > 16.0) {
    digitalWrite(ledPin[LED_BLUE1], HIGH);
    digitalWrite(ledPin[LED_BLUE2], HIGH);
  } else {
    digitalWrite(ledPin[LED_BLUE2], HIGH);
  }
  if (isChanged) {
    snprintf(buffer, sizeof(buffer), oledMessage[MSG_TEMP], tempC, tempF);
    displayMessage(buffer);
    isChanged = false;
  }
}

void modeHumidity() {
  // Execute the humidity sensor reading and display based on values ensuring appropriate delay between readings
  int isChanged = false;
  static int oldValue = 0;
  static int humidity = 0;
  static char buffer[32];
  if (isButtonPressed || lastDelayTime[DELAY_TMPHUM] == 0 || millis() - lastDelayTime[DELAY_TMPHUM] > hardwareDelay[DELAY_TMPHUM]) {
    humidity = round(dht.readHumidity());
    if (isButtonPressed || oldValue != humidity) {
      oldValue = humidity;
      isChanged = true;
    }
    lastDelayTime[DELAY_TMPHUM] = millis();
    DEBUG_PRINTLN("Mode: Humidity: ");
    DEBUG_PRINTLN(humidity);
  }
  digitalWrite(ledPin[LED_WHITE1], LOW);
  digitalWrite(ledPin[LED_WHITE2], HIGH);
  toggleLeds(2, LOW);
    // RangeLED:   2(>26), 23(24-26), 34(23-24), 45(21-23), 56(21-18), 67(16-18), 7(<16)
  toggleLeds(2, LOW);
  if (humidity > 60) {
    digitalWrite(ledPin[LED_BLUE2], HIGH);
  } else if (humidity >55) {
    digitalWrite(ledPin[LED_BLUE1], HIGH);
    digitalWrite(ledPin[LED_BLUE2], HIGH);
  } else if (humidity > 50) {
    digitalWrite(ledPin[LED_BLUE1], HIGH);
    digitalWrite(ledPin[LED_GREEN2], HIGH);
  } else if (humidity > 45) {
    digitalWrite(ledPin[LED_GREEN1], HIGH);
    digitalWrite(ledPin[LED_GREEN2], HIGH);
  } else if (humidity > 40) {
    digitalWrite(ledPin[LED_GREEN1], HIGH);
    digitalWrite(ledPin[LED_RED2], HIGH);
  } else if (humidity > 35) {
    digitalWrite(ledPin[LED_RED1], HIGH);
    digitalWrite(ledPin[LED_RED2], HIGH);
  } else {
    digitalWrite(ledPin[LED_RED1], HIGH);
  }
  if (isChanged) {
    snprintf(buffer, sizeof(buffer), oledMessage[MSG_HUMIDITY], (int)humidity);
    displayMessage(buffer);
    isChanged = false;
  } 

}

void modeTimer() {
  // Execute the sit/stand timer display and check time remaining (convert from ms to minutes)
  bool isChanged = false;
  static unsigned long oldValue = 0;
  static char buffer[32];
  static unsigned long timerRemaining = 0;
  static uint8_t numLedsLit = NUM_LEDS - MODE_LED_COUNT;
  unsigned long currentMillis = millis();
  unsigned long timeRemaining = (timeToStand > currentMillis) ? (timeToStand - currentMillis) : 0;
  unsigned long totalSitMillis = maxSitMinutes * 60000UL;
  numLedsLit = round((NUM_LEDS - MODE_LED_COUNT) * ((float)timeRemaining / totalSitMillis));
  numLedsLit = constrain(numLedsLit, 0, NUM_LEDS - MODE_LED_COUNT);
  timerRemaining = round(timeRemaining / 60000.0);
  if (isButtonPressed || oldValue != timerRemaining) {
    oldValue = timerRemaining;
    isChanged = true;
  }
  digitalWrite(ledPin[LED_WHITE1], HIGH);
  digitalWrite(ledPin[LED_WHITE2], HIGH);
  toggleLeds(MODE_LED_COUNT, LOW);
  for (int i = 0; i < numLedsLit; i++) { digitalWrite(ledPin[i + MODE_LED_COUNT], HIGH); }
  if (isChanged) {
    snprintf(buffer, sizeof(buffer), oledMessage[MSG_TIMER], (int)timerRemaining);
    displayMessage(buffer);
    isChanged = false;
  }
  if (isButtonPressed || lastDelayTime[DELAY_TIMER] == 0 || currentMillis - lastDelayTime[DELAY_TIMER] > hardwareDelay[DELAY_TIMER]) {
    lastDelayTime[DELAY_TIMER] = currentMillis;
    DEBUG_PRINT("Mode: Timer, Max Sit Minutes: ");
    DEBUG_PRINT(maxSitMinutes);
    DEBUG_PRINT(", Time to Stand:  ");
    DEBUG_PRINT(timeToStand);
    DEBUG_PRINT(", Minutes of Sitting Remaining: ");
    DEBUG_PRINTLN(timerRemaining);
    }
}

void modeCycle() {
  // Cycle through the various modes with the corresponding delay between changes
  static uint8_t currentMode = 0;
  if (lastDelayTime[DELAY_CYCLE] == 0 || millis() - lastDelayTime[DELAY_CYCLE] > hardwareDelay[DELAY_CYCLE]) {
    lastDelayTime[DELAY_CYCLE] = millis();
    DEBUG_PRINT("Mode: Cycle, Current Mode: ");
    DEBUG_PRINT(currentMode);
    modeSelected = currentMode;
    isButtonPressed = true;
    runMode();
    currentMode = (currentMode + 1) % (NUM_MODES - 1);
    modeSelected = MODE_CYCLE;
    DEBUG_PRINT(", Next Mode: ");
    DEBUG_PRINTLN(currentMode);
  }
}
/*
  XMAS Project

  Christmas project for work gift exchange. Simply circuit and logic
  to display Temperature, Humidity, and a Sit/Stand Timer using
  both LEDs and LCD

  Component Checklist:
  1x Arduino Uno
  1x TMP36
  2x 6mm Tacticle Button Swtich
  1x SW 200D Tilt Sensor
  1x LCD 16x2 (I2C): PCF8574-based 0x27 Address
  8x LED: 2xRed,2xGreen,2xBlue,2xWhite
  8x Resistors: 8x220Ohm

  The Circuit:
  LCD 16x2 (I2C)
  LCD 16x2 (I2C)
    GND     -> Arduino:GND
    VCC     -> Arduino:5V
    SDA     -> Arduino:A4
    SCL     -> Arduino:A5
  SW200D
    T1      -> Arduino:GND
    T2      -> Arduino:3 (INPUT_PULLUP)
  TMP36
    PWR     -> Arduino:5V
    VOUT    -> Arduino:A2 
    GND     -> Arduino:GND
  Button (Power)
    T1A     -> Arduino:4 (INPUT_PULLUP)
    T2A     -> Arduino:GND
  Button (Mode)
    T1A     -> Arduino:5 (INPUT_PULLUP)
    T2A     -> Arduino:GND
  LED_WHITE_1
    Anode   -> 220Ohm -> Arduino:6
    Cathode -> Arduino:GND
  LED_WHITE_2
    Anode   -> 220Ohm -> Arduino:7
    Cathode -> Arduino:GND
  LED_RED_1
    Anode   -> 220Ohm -> Arduino:8
    Cathode -> Arduino:GND
  LED_RED_2
    Anode   -> 220Ohm -> Arduino:9
    Cathode -> Arduino:GND
  LED_GREEN_1
    Anode   -> 220Ohm -> Arduino:10
    Cathode -> Arduino:GND
  LED_GREEN_2
    Anode   -> 220Ohm -> Arduino:11
    Cathode -> Arduino:GND
  LED_BLUE_1
    Anode   -> 220Ohm -> Arduino:12
    Cathode -> Arduino:GND
  LED_BLUE_2
    Anode   -> 220Ohm -> Arduino:13
    Cathode -> Arduino:GND

  Created 09.29.2024
  By Steve Cundy
  Modified 09.29.2024
  By Steve Cundy

  http://git link?

*/

// Include necessary libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// Define OLED display dimensions
#define SCREEN_WIDTH 128 // OLED display width in pixels
#define SCREEN_HEIGHT 64 // OLED display height in pixels

// Define OLED reset pin (if necessary)
#define OLED_RESET -1 // Set to -1 if sharing Arduino reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define DHT sensor type and pin
#define DHTPIN 2     // DHT22 data pin connected to D2
#define DHTTYPE DHT22
//DHT dht(DHTPIN, DHTTYPE);

// Define pins for buttons and sensors
const int modeButtonPin = 3;        // Mode button connected to D3
const int modeIndicatorLEDPin = 4;  // Mode indicator LED connected to D4
const int powerButtonPin = 5;       // Power control button connected to D5
const int tiltSensorPin = 6;        // Tilt sensor connected to D6
const int LDRPin = A0;              // Photoresistor connected to A0

// Define pins for LEDs
const int redLEDPins[2] = {7, 8};    // Red LEDs connected to D7 and D8
const int greenLEDPins[2] = {9, 10}; // Green LEDs connected to D9 and D10
const int blueLEDPins[2] = {11, 12}; // Blue LEDs connected to D11 and D12

// Variables to store button states
bool modeButtonPressed = false;
bool powerButtonPressed = false;

// Variables for modes
enum Mode { TEMPERATURE, HUMIDITY, SIT_STAND };
Mode currentMode = TEMPERATURE;

// Variables for power control
bool powerOn = true; // Start with power on

// Variables for sit/stand timer
unsigned long sitStandInterval = 3600000; // 1 hour in milliseconds
unsigned long lastStandTime = 0;

// Variables for tilt sensor
bool tiltDetected = false;

// Variables for LDR
int LDRValue = 0; // Light level

void setup() {
  // Initialize serial communication (optional for debugging)
  Serial.begin(9600);

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Loop forever if display initialization fails
  }
  display.clearDisplay();
  display.display();

  // Initialize DHT sensor
  dht.begin();

  // Set pin modes for buttons and sensors
  pinMode(modeButtonPin, INPUT_PULLUP);  // Mode button with internal pull-up
  pinMode(powerButtonPin, INPUT_PULLUP); // Power button with internal pull-up
  pinMode(tiltSensorPin, INPUT_PULLUP);  // Tilt sensor with internal pull-up

  // Set pin modes for LEDs
  pinMode(modeIndicatorLEDPin, OUTPUT);
  digitalWrite(modeIndicatorLEDPin, LOW); // Turn off mode indicator LED

  for (int i = 0; i < 2; i++) {
    pinMode(redLEDPins[i], OUTPUT);
    digitalWrite(redLEDPins[i], LOW); // Turn off red LEDs
    pinMode(greenLEDPins[i], OUTPUT);
    digitalWrite(greenLEDPins[i], LOW); // Turn off green LEDs
    pinMode(blueLEDPins[i], OUTPUT);
    digitalWrite(blueLEDPins[i], LOW); // Turn off blue LEDs
  }

  // Initialize sit/stand timer
  lastStandTime = millis();
}

void loop() {
  // Read buttons and sensors
  readButtons();
  readTiltSensor();
  readLDR();

  // Power control
  if (powerOn) {
    // Update display and LEDs based on the current mode
    switch (currentMode) {
      case TEMPERATURE:
        displayTemperature();
        updateTemperatureLEDs();
        break;
      case HUMIDITY:
        displayHumidity();
        updateHumidityLEDs();
        break;
      case SIT_STAND:
        displaySitStandTimer();
        updateSitStandLEDs();
        break;
    }
  } else {
    // Turn off display and LEDs
    display.clearDisplay();
    display.display();
    turnOffAllLEDs();
  }

  // Small delay to debounce buttons and reduce CPU usage
  delay(100);
}

void readButtons() {
  // Read mode button
  if (digitalRead(modeButtonPin) == LOW && !modeButtonPressed) {
    modeButtonPressed = true;
    // Change mode
    currentMode = static_cast<Mode>((currentMode + 1) % 3);
    // Flash mode indicator LED
    flashModeIndicator();
  }
  if (digitalRead(modeButtonPin) == HIGH && modeButtonPressed) {
    modeButtonPressed = false;
  }

  // Read power button
  if (digitalRead(powerButtonPin) == LOW && !powerButtonPressed) {
    powerButtonPressed = true;
    // Toggle power state
    powerOn = !powerOn;
  }
  if (digitalRead(powerButtonPin) == HIGH && powerButtonPressed) {
    powerButtonPressed = false;
  }
}

void readTiltSensor() {
  if (digitalRead(tiltSensorPin) == LOW) {
    tiltDetected = true;
    // Handle tilt event (e.g., reset sit/stand timer)
    lastStandTime = millis();
  } else {
    tiltDetected = false;
  }
}

void readLDR() {
  // Read LDR value (0-1023)
  LDRValue = analogRead(LDRPin);
  // Adjust display brightness based on ambient light
  // Map LDR value to contrast level (optional)
  // int contrast = map(LDRValue, 0, 1023, 0, 255);
  // display.ssd1306_command(SSD1306_SETCONTRAST);
  // display.ssd1306_command(contrast);
}

void displayTemperature() {
  // Clear the display
  display.clearDisplay();

  // Read temperature from DHT22
  float temperature = dht.readTemperature(); // Celsius
  if (isnan(temperature)) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Error reading temperature!"));
  } else {
    // Display temperature
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.print("Temp: ");
    display.print(temperature, 1);
    display.print(" C");
  }

  // Update the display
  display.display();
}

void displayHumidity() {
  // Clear the display
  display.clearDisplay();

  // Read humidity from DHT22
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Error reading humidity!"));
  } else {
    // Display humidity
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.print("Hum: ");
    display.print(humidity, 1);
    display.print(" %");
  }

  // Update the display
  display.display();
}

void displaySitStandTimer() {
  // Clear the display
  display.clearDisplay();

  // Calculate time left before standing
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastStandTime;
  unsigned long timeLeft = (sitStandInterval - elapsedTime) / 1000; // In seconds

  if (elapsedTime >= sitStandInterval) {
    // Time to stand up
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("Stand Up!");
    // You can add additional alerts here (e.g., flash LEDs)
  } else {
    // Display time left
    int minutes = timeLeft / 60;
    int seconds = timeLeft % 60;
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Time Left:");
    display.setCursor(0, 30);
    display.print(minutes);
    display.print("m ");
    display.print(seconds);
    display.print("s");
  }

  // Update the display
  display.display();
}

void updateTemperatureLEDs() {
  // Turn on LEDs based on temperature range
  float temperature = dht.readTemperature(); // Celsius
  if (isnan(temperature)) {
    turnOffAllLEDs();
    return;
  }

  // Example logic: Light up more LEDs as temperature increases
  turnOffAllLEDs();
  if (temperature > 25) {
    digitalWrite(redLEDPins[0], HIGH);
  }
  if (temperature > 30) {
    digitalWrite(redLEDPins[1], HIGH);
  }
}

void updateHumidityLEDs() {
  // Turn on LEDs based on humidity range
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    turnOffAllLEDs();
    return;
  }

  // Example logic: Light up more LEDs as humidity increases
  turnOffAllLEDs();
  if (humidity > 50) {
    digitalWrite(greenLEDPins[0], HIGH);
  }
  if (humidity > 70) {
    digitalWrite(greenLEDPins[1], HIGH);
  }
}

void updateSitStandLEDs() {
  // Use blue LEDs to indicate time left before standing
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastStandTime;
  unsigned long timeLeft = sitStandInterval - elapsedTime;

  turnOffAllLEDs();
  if (timeLeft < sitStandInterval / 2) {
    digitalWrite(blueLEDPins[0], HIGH);
  }
  if (timeLeft < sitStandInterval / 4) {
    digitalWrite(blueLEDPins[1], HIGH);
  }

  // If it's time to stand up, reset the timer
  if (elapsedTime >= sitStandInterval) {
    // You can add additional alerts here
    lastStandTime = millis(); // Reset the timer after standing
  }
}

void turnOffAllLEDs() {
  // Turn off all LEDs
  digitalWrite(modeIndicatorLEDPin, LOW);
  for (int i = 0; i < 2; i++) {
    digitalWrite(redLEDPins[i], LOW);
    digitalWrite(greenLEDPins[i], LOW);
    digitalWrite(blueLEDPins[i], LOW);
  }
}

void flashModeIndicator() {
  // Flash the mode indicator LED three times
  for (int i = 0; i < 3; i++) {
    digitalWrite(modeIndicatorLEDPin, HIGH);
    delay(200);
    digitalWrite(modeIndicatorLEDPin, LOW);
    delay(200);
  }
}

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
// Define PINS
const int dht22SensorPin = A2;  //dht22 Need digital pin 2
const int tiltSensorPin = 3;
const int buttonPin[] = { 4, 5 };                     // 0=Power,1=Mode
const int ledPin[] = { 6, 7, 8, 9, 10, 11, 12, 13 };  // 0=Mode1,1=Mode2,23=Red,45=Green,67=Blue
// const int screenPin

//Define Constants
const int ledNum = sizeof(ledPin) / sizeof(ledPin[0]);
const int switchNum = sizeof(buttonPin) / sizeof(buttonPin[0]);
const int ledTimerNum = ledNum - 2;
const unsigned long hardwareDelay[] = { 250, 250, 1000, 500, 60000, 1000 };  //0=Power,1=Mode,2=DHT22,3=Tilt,4=Timer,5=Cycle

//System Variables
unsigned long lastDelayTime[] = { 0, 0, 0, 0, 0, 0 };  //0=Power,1=Mode,2=DHT22,3=Tilt,4=Timer,5=Cycle
float voltage, tempC, tempF;
int ledState = LOW;
int tiltState = LOW;
bool pwrSelected = true;
int timerRemaining = 0;
int numModes = 4;
int currentMode = 0;
int ledLit = ledNum;

//User Variables
bool debugEnable = true;  // Default false
int ledBlinkDelay = 50;   // Default 500
int ledFlashNum = 0;      // Default 3
int modeSelected = 0;     // Default: 0; First Mode Selectded: 0=Temperature,1=Humidity,2=Timer,3=Cycle
int maxSitMinutes = 5;    // Default: 30

//Function prototype
void chkModeButton(bool modeOverride = false, int newMode = 0);
//void updateLEDsByTemp(float tempC);

void setup() {
  if (debugEnable) { Serial.begin(9600); }
  // Set Pin Modes
  for (int i = 0; i < ledNum; i++) { pinMode(ledPin[i], OUTPUT); }
  for (int i = 0; i < switchNum; i++) { pinMode(buttonPin[i], INPUT_PULLUP); }
  //pinMode(dht22SensorPin, INPUT_PULLUP);
  pinMode(tiltSensorPin, INPUT_PULLUP);

  startup();
}

void loop() {
  chkPowerButton();
  chkModeButton();
  chkTiltSensor();
  modeTemp();
  modeHumidity();
  modeTimer();
  modeCycle();
}

void startup() {
  ledState = HIGH;
  for (int i = 0; i < ledNum; i++) {
    digitalWrite(ledPin[i], ledState);
    delay(ledBlinkDelay);
  }
  for (int i = 0; i < ledNum; i++) { digitalWrite(ledPin[i], LOW); }
  delay(ledBlinkDelay);
  ledFlashAll(ledFlashNum);
}

void shutdown() {
  for (int i = 0; i < ledNum; i++) { digitalWrite(ledPin[i], LOW); }
  delay(ledBlinkDelay);
  ledFlashAll(ledFlashNum);
  ledState = LOW;
  for (int i = 0; i < ledNum; i++) { digitalWrite(ledPin[i], HIGH); }
  delay(ledBlinkDelay);
  for (int i = (ledNum - 1); i >= 0; i--) {
    digitalWrite(ledPin[i], ledState);
    delay(ledBlinkDelay);
  }
}

void ledFlashAll(int numFlash) {
  ledState = HIGH;
  for (int j = 0; j < (numFlash * 2); j++) {
    for (int i = 0; i < ledNum; i++) { digitalWrite(ledPin[i], ledState); }
    ledState = (ledState == LOW) ? HIGH : LOW;
    delay(ledBlinkDelay);
  }
}

void chkPowerButton() {
  int pwrState = digitalRead(buttonPin[0]);
  if (pwrState == LOW && (millis() - lastDelayTime[0] > hardwareDelay[0])) {
    pwrSelected = !pwrSelected;
    if (pwrSelected) {
      startup();
    } else {
      shutdown();
    }
    lastDelayTime[0] = millis();
  }
}

void chkModeButton(bool modeOverride, int newMode) {
  int modeState = digitalRead(buttonPin[1]);
  if ((modeState == LOW && pwrSelected && (millis() - lastDelayTime[1] > hardwareDelay[1])) || modeOverride) {
    if (modeOverride) {
      modeSelected = newMode;
    } else {
      modeSelected = (modeSelected + 1) % numModes;
      lastDelayTime[1] = millis();
    }
    if (debugEnable) {
      Serial.print("Mode Button Pressed: ");
      Serial.println(modeSelected);
    }
    switch (modeSelected) {
      case 0: modeTemp(); break;
      case 1: modeHumidity(); break;
      case 2: modeTimer(); break;
      case 3: modeCycle(); break;
    }
  }
}

void chkTiltSensor() {
  if (lastDelayTime[3] == 0 || millis() - lastDelayTime[3] > hardwareDelay[3]) {
    int newTiltState = digitalRead(tiltSensorPin);
    if (tiltState != newTiltState) {
      if (debugEnable) { Serial.println("Tilt Detected!"); }
      lastDelayTime[4] = 0;
      timerRemaining = maxSitMinutes;
      tiltState = newTiltState;
    }
    lastDelayTime[3] = millis();
  }
}

void modeTemp() {
  if (!modeSelected && pwrSelected) {
    digitalWrite(ledPin[0], HIGH);
    digitalWrite(ledPin[1], LOW);
    // RangeLED:   2(>26), 23(24-26), 34(23-24), 45(21-23), 56(21-18), 67(16-18), 7(<16)
    for (int i = 2; i < ledNum; i++) { digitalWrite(ledPin[i], LOW); }
    if (tempC > 26.0) {
      digitalWrite(ledPin[2], HIGH);
    } else if (tempC > 24.0 && tempC <= 26.0) {
      digitalWrite(ledPin[2], HIGH);
      digitalWrite(ledPin[3], HIGH);
    } else if (tempC > 23.0 && tempC <= 24.0) {
      digitalWrite(ledPin[3], HIGH);
      digitalWrite(ledPin[4], HIGH);
    } else if (tempC > 21.0 && tempC <= 23.0) {
      digitalWrite(ledPin[4], HIGH);
      digitalWrite(ledPin[5], HIGH);
    } else if (tempC > 18.0 && tempC <= 21.0) {
      digitalWrite(ledPin[5], HIGH);
      digitalWrite(ledPin[6], HIGH);
    } else if (tempC > 16.0 && tempC <= 18.0) {
      digitalWrite(ledPin[6], HIGH);
      digitalWrite(ledPin[7], HIGH);
    } else if (tempC <= 16.0) {
      digitalWrite(ledPin[7], HIGH);
    }
    if (lastDelayTime[2] == 0 || millis() - lastDelayTime[2] > hardwareDelay[2]) {
      int sensorValue = analogRead(dht22SensorPin);
      voltage = sensorValue * (5.0 / 1024.0);
      tempC = (voltage - 0.5) * 100.0;
      tempF = (tempC * 9.0 / 5.0) + 32.0;
      lastDelayTime[2] = millis();
      if (debugEnable) {
        Serial.println("Mode: Temperature");
        Serial.print("Voltage: ");
        Serial.print(voltage);
        Serial.print(", Temperature: ");
        Serial.print(tempC);
        Serial.print("C, ");
        Serial.println(tempF);
      }
    }
  }
}

void modeHumidity() {
  if (modeSelected == 1 && pwrSelected) {
    digitalWrite(ledPin[0], LOW);
    digitalWrite(ledPin[1], HIGH);
    for (int i = 2; i < ledNum; i++) { digitalWrite(ledPin[i], LOW); }
    if (lastDelayTime[2] == 0 || millis() - lastDelayTime[2] > hardwareDelay[2]) {
      int sensorValue = analogRead(dht22SensorPin);
      lastDelayTime[2] = millis();
      if (debugEnable) { Serial.println("Mode: Humidity"); }
    }
  }
}

void modeTimer() {
  if (modeSelected == 2 && pwrSelected) {
    digitalWrite(ledPin[0], HIGH);
    digitalWrite(ledPin[1], HIGH);
    for (int i = 2; i < ledNum; i++) { digitalWrite(ledPin[i], LOW); }
    if (timerRemaining) {
      for (int i = 0; i < ledLit; i++) { digitalWrite(ledPin[i + 2], HIGH); }
    }
    if (lastDelayTime[4] == 0 || millis() - lastDelayTime[4] > hardwareDelay[4]) {
      lastDelayTime[4] = millis();
      ledLit = (float)ledTimerNum * ((float)timerRemaining / (float)maxSitMinutes);
      if (debugEnable) {
        Serial.println("Mode: Timer");
        Serial.print("Timer Remaining: ");
        Serial.println(timerRemaining);
        Serial.print("Max Sit Minutes: ");
        Serial.println(maxSitMinutes);
        Serial.print("LED Num: ");
        Serial.println(ledTimerNum);
        Serial.print("LED Lit: ");
        Serial.println(ledLit);
        Serial.print("Minutes of Sitting Remaining: ");
        Serial.println(timerRemaining);
      }
      if (timerRemaining) { timerRemaining--; }
    }
  }
}

void modeCycle() {
  if (modeSelected == 3 && pwrSelected) {
    if (lastDelayTime[5] == 0 || millis() - lastDelayTime[5] > hardwareDelay[5]) {
      lastDelayTime[5] = millis();
      if (debugEnable) {
        Serial.println("Mode: Cycle");
        Serial.print("Current Mode: ");
        Serial.println(currentMode);
      }
      chkModeButton(true, currentMode);
      currentMode = (currentMode + 1) % (numModes - 1);
      modeSelected = 3;
      if (debugEnable) {
        Serial.print("Mode: Cycle Next ");
        Serial.println(currentMode);
      }
    }
  }
}
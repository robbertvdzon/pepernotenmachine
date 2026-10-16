#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

const int MAX = 25;//37
const int CENTER = 35;//30
const int MIN = 10;


// Inputs:
// D2 connected to GND -> 50%
// D3 connected to GND -> 25%
// Nothing connected    -> 0%
const int PIN_50 = 2;
const int PIN_25 = 3;

// Servo pulse range.
// If the servo hits its mechanical stop, reduce this range.
const int SERVO_MIN = 0;
const int SERVO_MAX = 1000;

// Normal servo channel after the startup test.
const int SERVO_CHANNEL = 0;

void setup() {
  pinMode(PIN_50, INPUT_PULLUP);
  pinMode(PIN_25, INPUT_PULLUP);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(100);

  // Startup test:
  // Move ALL 16 PCA9685 channels left/right twice.
  // for (int i = 0; i < 2; i++) {
    // setAllServoPercent(0);
    // delay(700);

    // setAllServoPercent(100);
    // delay(700);
  // }

  // Finish startup test at 0%.
  setServoPercent(SERVO_CHANNEL, CENTER);
  delay(500);
}

// While D2 is LOW: go to CENTER, return after 500 ms, wait 3 s, repeat.
const unsigned long CENTER_TIME_MS = 500;
const unsigned long PAUSE_TIME_MS = 3000;

bool pin50Active = false;
bool atCenter = false;
unsigned long phaseStart = 0;

void loop() {
  if (digitalRead(PIN_50) != LOW) {
    pin50Active = false;
  }

  if (digitalRead(PIN_50) == LOW) {
    unsigned long now = millis();

    if (!pin50Active) {
      // D2 just went LOW: start with a move to CENTER.
      pin50Active = true;
      atCenter = true;
      phaseStart = now;
      setServoPercent(SERVO_CHANNEL, CENTER);
    }
    else if (atCenter && now - phaseStart >= CENTER_TIME_MS) {
      // 500 ms at CENTER: go back.
      atCenter = false;
      phaseStart = now;
      setServoPercent(SERVO_CHANNEL, MIN);
    }
    else if (!atCenter && now - phaseStart >= PAUSE_TIME_MS) {
      // 3 s after going back: CENTER again.
      atCenter = true;
      phaseStart = now;
      setServoPercent(SERVO_CHANNEL, CENTER);
    }
  }
  else if (digitalRead(PIN_25) == LOW) {
    setServoPercent(SERVO_CHANNEL, CENTER);
  }
  else {
    setServoPercent(SERVO_CHANNEL, MIN);
  }

  delay(20);
}

void setServoPercent(int channel, int percent) {
  percent = constrain(percent, 0, 100);

  int pulse = map(percent, 0, 100, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(channel, 0, pulse);
}

void setAllServoPercent(int percent) {
  for (int channel = 0; channel < 16; channel++) {
    setServoPercent(channel, percent);
  }
}


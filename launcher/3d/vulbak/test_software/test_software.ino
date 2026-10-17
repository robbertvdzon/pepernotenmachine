#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// While D2 is LOW: go to CENTER, return after 2 s, wait 5 s, repeat.
const unsigned long CENTER_TIME_MS = 400;
const unsigned long PAUSE_TIME_MS = 5000;


const int CENTER = 28;//28
const int MIN = 15;//10


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

// Second servo runs mirrored (contra) to the first one, within the MIN..CENTER range.
const int SERVO_CHANNEL_2 = 1;

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
  setServos(CENTER);
  delay(500);
}


// D2 only counts as released after it stayed HIGH this long (switch bounce).
const unsigned long DEBOUNCE_MS = 50;

bool pin50Pressed = false;
unsigned long pin50LastLow = 0;

bool pin50Active = false;
bool atCenter = false;
unsigned long phaseStart = 0;

void loop() {
  unsigned long now = millis();

  if (digitalRead(PIN_50) == LOW) {
    pin50Pressed = true;
    pin50LastLow = now;
  }
  else if (pin50Pressed && now - pin50LastLow >= DEBOUNCE_MS) {
    pin50Pressed = false;
    pin50Active = false;
  }

  if (pin50Pressed) {
    if (!pin50Active) {
      // D2 just went LOW: start with a move to CENTER.
      pin50Active = true;
      atCenter = true;
      phaseStart = now;
      setServos(CENTER);
    }
    else if (atCenter && now - phaseStart >= CENTER_TIME_MS) {
      // 500 ms at CENTER: go back.
      atCenter = false;
      phaseStart = now;
      setServos(MIN);
    }
    else if (!atCenter && now - phaseStart >= PAUSE_TIME_MS) {
      // 3 s after going back: CENTER again.
      atCenter = true;
      phaseStart = now;
      setServos(CENTER);
    }
  }
  else if (digitalRead(PIN_25) == LOW) {
    setServos(CENTER);
  }
  else {
    setServos(MIN);
  }

  delay(20);
}

void setServoPercent(int channel, int percent) {
  percent = constrain(percent, 0, 100);

  int pulse = map(percent, 0, 100, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(channel, 0, pulse);
}

// Sets servo 1 to percent and servo 2 to the mirrored position:
// MIN <-> CENTER swapped, so servo 2 stays within the usable pulse range.
void setServos(int percent) {
  setServoPercent(SERVO_CHANNEL, percent);
  setServoPercent(SERVO_CHANNEL_2, MIN + CENTER - percent);
}

void setAllServoPercent(int percent) {
  for (int channel = 0; channel < 16; channel++) {
    setServoPercent(channel, percent);
  }
}


#ifndef TYPES_H
#define TYPES_H

// State structure to hold all system state
struct SystemState {
  int tempTarget;
  int humTarget;
  int menuIndex;
  float humidity;
  float temperature;
  bool sensorReadSuccess;
  unsigned long lastButtonPress;
  unsigned long lastSensorRead;
  int lastEncoderValue;
  unsigned long buttonPressStart;    // When button was first pressed
  unsigned long timerSeconds;        // Timer countdown in seconds (remaining time)
  unsigned long timerOriginalSeconds; // Original timer duration
  unsigned long timerStartTime;      // When timer was started (millis())
  bool timerRunning;                 // Whether timer is actively counting down
};

// Fan PWM state structure
struct FanPwmState {
  unsigned long lastCycleStart;
  unsigned long period;
  bool isOn;
};

// Heater PWM state structure
struct HeaterPwmState {
  unsigned long lastCycleStart;
  unsigned long period;
  bool isOn;
};

#endif // TYPES_H 
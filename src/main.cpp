#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#include "AiEsp32RotaryEncoder.h"

// Include our modular headers
#include "config.h"
#include "types.h"
#include "sensors.h"
#include "controls.h"
#include "display.h"
#include "input.h"
#include "timer.h"
#include "persistence.h"

// Hardware initialization
DHT dht(DHTPIN, DHTTYPE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW, -1, ROTARY_ENCODER_STEPS);

// Function prototypes
void setupHardware();
SystemState createInitialState();

// Global state that can't be easily made functional due to hardware interactions
SystemState state;
FanPwmState fanState = {0, 1000/FAN_PWM_FREQ_SOFT, false};  // Calculate proper period from frequency
HeaterPwmState heaterState = {0, 1000/FAN_PWM_FREQ_SOFT, false};  // Use same period for heater

void setup() {
  setupHardware();
  state = createInitialState();
  
  // Load stored settings from preferences
  state = loadStoredSettings(state);
  
  state = readSensors(state);
}

void loop() {
  // Read sensors periodically based on time
  if (millis() - state.lastSensorRead >= SENSOR_READ_INTERVAL) {
    state = readSensors(state);
  }

  // Process inputs and update state in a functional way
  state = processEncoder(state);
  state = processButton(state);
  state = clampValues(state);
  state = updateTimer(state);
  
  // Calculate outputs based on state
  int fanPwm = calculateFanSpeed(state);
  int heaterPwm = calculateHeaterPower(state);
  updateDisplay(state);
  
  // Update fan and heater state and apply to hardware
  fanState = updateFanPwm(fanPwm, fanState);
  heaterState = updateHeaterPwm(heaterPwm, heaterState);
  applyFanOutput(fanState.isOn);
  applyHeaterOutput(heaterState.isOn);
  
  delay(5); // Small delay to help with debouncing
}

void setupHardware() {
  Serial.begin(9600);
  delay(300);
  
  Wire.begin(8, 9);
  u8g2.begin();
  dht.begin();

  // Initialize rotary encoder
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  bool circleValues = false;
  rotaryEncoder.setBoundaries(TEMP_MIN, TEMP_MAX, circleValues);
  rotaryEncoder.setAcceleration(50);
  
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
}

SystemState createInitialState() {
  SystemState newState = {
    .tempTarget = 10,
    .humTarget = 50,
    .menuIndex = 0,
    .humidity = 0,
    .temperature = 0,
    .sensorReadSuccess = false,
    .lastButtonPress = 0,
    .lastSensorRead = 0,
    .lastEncoderValue = 10,
    .buttonPressStart = 0,
    .timerSeconds = 0,
    .timerOriginalSeconds = 0,
    .timerStartTime = 0,
    .timerRunning = false
  };
  
  rotaryEncoder.setEncoderValue(newState.tempTarget);
  
  return newState;
} 
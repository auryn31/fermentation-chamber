#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
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
Adafruit_BME280 bme;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW, -1, ROTARY_ENCODER_STEPS);

// Function prototypes
void setupHardware();
SystemState createInitialState();

// Global state that can't be easily made functional due to hardware interactions
SystemState state;
FanPwmState fanState = {0, 1000/FAN_PWM_FREQ_SOFT, false, 0};  // Calculate proper period from frequency
HeaterPwmState heaterState = {0, 1000/FAN_PWM_FREQ_SOFT, false};  // Use same period for heater
VaporizerState vaporizerState = {false, 0};

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
  int fanPwm = calculateFanSpeed(state, vaporizerState);
  int heaterPwm = calculateHeaterPower(state);
  bool vaporizerOn = calculateVaporizerState(state, vaporizerState);
  updateDisplay(state, vaporizerState);
  
  // Update fan and heater state and apply to hardware
  fanState = updateFanPwm(fanPwm, fanState);
  heaterState = updateHeaterPwm(heaterPwm, heaterState);
  applyFanOutput(fanState.isOn);
  applyHeaterOutput(heaterState.isOn);
  applyVaporizerOutput(vaporizerOn);
  
  // Update vaporizer state
  if (vaporizerOn != vaporizerState.isOn) {
    vaporizerState.isOn = vaporizerOn;
    vaporizerState.lastStateChange = millis();
  }
  
  // Debug output every 2 seconds
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    Serial.print("FanPWM: ");
    Serial.print(fanPwm);
    Serial.print(", FanOn: ");
    Serial.print(fanState.isOn);
    Serial.print(", Period: ");
    Serial.print(fanState.period);
    
    // Calculate timing values for debugging
    unsigned long now = millis();
    unsigned long cycleTime = now - fanState.lastCycleStart;
    unsigned long onTime = (fanPwm * fanState.period) / 255;
    
    Serial.print(", CycleTime: ");
    Serial.print(cycleTime);
    Serial.print(", OnTime: ");
    Serial.print(onTime);
    Serial.print(", Temp: ");
    Serial.print(state.temperature);
    Serial.print(", Target: ");
    Serial.print(state.tempTarget);
    Serial.print(", Humidity: ");
    Serial.print(state.humidity);
    Serial.print(", HumTarget: ");
    Serial.print(state.humTarget);
    Serial.print(", Vaporizer: ");
    Serial.println(vaporizerOn);
    lastDebug = millis();
  }
  
  delay(1); // Small delay to help with debouncing
}

void setupHardware() {
  Serial.begin(9600);
  delay(300);
  
  Wire.begin(8, 9);
  u8g2.begin();
  
  // Initialize BME280 sensor
  if (!bme.begin(BME280_I2C_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
  } else {
    Serial.println("BME280 sensor found and initialized!");
  }

  // Initialize rotary encoder
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  bool circleValues = false;
  rotaryEncoder.setBoundaries(TEMP_MIN, TEMP_MAX, circleValues);
  rotaryEncoder.setAcceleration(50);
  
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(VAPORIZER_PIN, OUTPUT);
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
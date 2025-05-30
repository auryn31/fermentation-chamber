#include <Arduino.h>
#include "controls.h"
#include "config.h"

int calculateFanSpeed(const SystemState& state) {
  // Pure function that calculates fan speed based on state
  int fanPwm = FAN_PWM_MIN;  // Default: minimal speed
  
  if (state.sensorReadSuccess) {
    float tempDiff = state.temperature - state.tempTarget;  // Positive when too hot
    float humDiff = state.humidity - state.humTarget;       // Positive when too humid
    float tempDeficit = state.tempTarget - state.temperature; // Positive when too cold
    
    if (tempDiff > 0) {
      // Too hot: speed up fan linearly, max at +10C
      fanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * min(tempDiff, 10.0f) / 10.0f);
    } else if (humDiff > 0) {
      // Humidity too high: fan should be on
      if (tempDeficit >= TEMP_THRESHOLD_LOW) {
        // Too cold and too humid: minimal fan speed (heater will also be on)
        fanPwm = FAN_PWM_MIN;
      } else {
        // Not too cold but too humid: higher fan speed
        fanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * min(humDiff, 50.0f) / 50.0f);
      }
    } else if (tempDeficit >= TEMP_THRESHOLD_LOW) {
      // Too cold but humidity OK: minimal fan speed (heater will be on)
      fanPwm = FAN_PWM_MIN;
    }
  }
  
  return fanPwm;
}

int calculateHeaterPower(const SystemState& state) {
  // Pure function that calculates heater power based on state
  int heaterPwm = 0;  // Default: off
  
  if (state.sensorReadSuccess) {
    float tempDiff = state.tempTarget - state.temperature;  // Positive when too cold
    
    // Debug output to help diagnose heater always on issue
    Serial.print("HEATER DEBUG - Target: "); Serial.print(state.tempTarget);
    Serial.print("°C, Current: "); Serial.print(state.temperature);
    Serial.print("°C, TempDiff: "); Serial.print(tempDiff);
    Serial.print("°C, Threshold: "); Serial.print(TEMP_THRESHOLD_LOW);
    
    // Heater should be on only if temperature is below target by threshold
    if (tempDiff >= TEMP_THRESHOLD_LOW) {
      // Too cold: turn on heater proportionally, max at 5°C below target
      heaterPwm = HEATER_PWM_MIN + int((HEATER_PWM_MAX - HEATER_PWM_MIN) * min(tempDiff, 5.0f) / 5.0f);
      Serial.print("°C -> HEATER ON, PWM: "); Serial.println(heaterPwm);
    } else {
      Serial.println("°C -> HEATER OFF");
    }
  } else {
    Serial.println("HEATER DEBUG - No sensor reading, heater OFF");
  }
  
  return heaterPwm;
}

FanPwmState updateFanPwm(int fanPwmValue, const FanPwmState& pwmState) {
  // Compute new PWM state based on current state and value
  FanPwmState newState = pwmState;
  unsigned long now = millis();
  unsigned long cycleTime = now - pwmState.lastCycleStart;

  // Start a new PWM period if needed
  if (cycleTime >= pwmState.period) {
    newState.lastCycleStart = now;
    cycleTime = 0;
  }

  unsigned long onTime = (fanPwmValue * pwmState.period) / 255;
  newState.isOn = (cycleTime < onTime);
  
  #ifdef DEBUG_PWM
  Serial.print("PWM: "); Serial.print(fanPwmValue);
  Serial.print(" On: "); Serial.println(newState.isOn);
  #endif
  
  return newState;
}

HeaterPwmState updateHeaterPwm(int heaterPwmValue, const HeaterPwmState& pwmState) {
  // Compute new PWM state based on current state and value
  HeaterPwmState newState = pwmState;
  unsigned long now = millis();
  unsigned long cycleTime = now - pwmState.lastCycleStart;

  // Start a new PWM period if needed
  if (cycleTime >= pwmState.period) {
    newState.lastCycleStart = now;
    cycleTime = 0;
  }

  unsigned long onTime = (heaterPwmValue * pwmState.period) / 255;
  newState.isOn = (cycleTime < onTime);
  
  #ifdef DEBUG_PWM
  Serial.print("PWM: "); Serial.print(heaterPwmValue);
  Serial.print(" On: "); Serial.println(newState.isOn);
  #endif
  
  return newState;
}

void applyFanOutput(bool isOn) {
  // Hardware output side effect isolated to this function
  digitalWrite(FAN_PIN, isOn ? HIGH : LOW);
}

void applyHeaterOutput(bool isOn) {
  // Hardware output side effect isolated to this function
  digitalWrite(HEATER_PIN, isOn ? HIGH : LOW);
} 
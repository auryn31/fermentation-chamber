#include <Arduino.h>
#include "controls.h"
#include "config.h"

int calculateFanSpeed(const SystemState& state) {
  int fanPwm = FAN_PWM_MIN;
  
  if (state.sensorReadSuccess) {
    float tempDiff = state.temperature - state.tempTarget;
    float humDiff = state.humidity - state.humTarget;
    float tempDeficit = state.tempTarget - state.temperature;
    
    if (tempDiff > 0) {
      fanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * min(tempDiff, 10.0f) / 10.0f);
    } else if (humDiff > 0) {
      if (tempDeficit >= TEMP_THRESHOLD_LOW) {
        fanPwm = FAN_PWM_MIN;
      } else {
        fanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * min(humDiff, 50.0f) / 50.0f);
      }
    } else if (tempDeficit >= TEMP_THRESHOLD_LOW) {
      fanPwm = FAN_PWM_MIN;
    }
  }
  
  return fanPwm;
}

int calculateHeaterPower(const SystemState& state) {
  int heaterPwm = 0;
  
  if (state.sensorReadSuccess) {
    float tempDiff = state.tempTarget - state.temperature;
    
    if (tempDiff >= TEMP_THRESHOLD_LOW) {
      heaterPwm = HEATER_PWM_MIN + int((HEATER_PWM_MAX - HEATER_PWM_MIN) * min(tempDiff, 2.0f) / 2.0f);
    }
  }
  
  return heaterPwm;
}

FanPwmState updateFanPwm(int fanPwmValue, const FanPwmState& pwmState) {
  FanPwmState newState = pwmState;
  unsigned long now = millis();
  unsigned long cycleTime = now - pwmState.lastCycleStart;

  if (cycleTime >= pwmState.period) {
    newState.lastCycleStart = now;
    cycleTime = 0;
  }

  unsigned long onTime = (fanPwmValue * pwmState.period) / 255;
  newState.isOn = (cycleTime < onTime);
  
  return newState;
}

HeaterPwmState updateHeaterPwm(int heaterPwmValue, const HeaterPwmState& pwmState) {
  HeaterPwmState newState = pwmState;
  unsigned long now = millis();
  unsigned long cycleTime = now - pwmState.lastCycleStart;

  if (cycleTime >= pwmState.period) {
    newState.lastCycleStart = now;
    cycleTime = 0;
  }

  unsigned long onTime = (heaterPwmValue * pwmState.period) / 255;
  newState.isOn = (cycleTime < onTime);
  
  return newState;
}

void applyFanOutput(bool isOn) {
  digitalWrite(FAN_PIN, isOn ? HIGH : LOW);
}

void applyHeaterOutput(bool isOn) {
  digitalWrite(HEATER_PIN, isOn ? HIGH : LOW);
} 
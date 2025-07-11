#include <Arduino.h>
#include "controls.h"
#include "config.h"

int calculateFanSpeed(const SystemState& state, const VaporizerState& vaporizerState) {
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
        int baseFanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * min(humDiff, 50.0f) / 50.0f);
        if (humDiff > 2.0f && !vaporizerState.isOn) {
          fanPwm = min(FAN_PWM_MAX, baseFanPwm + 50);
        } else {
          fanPwm = baseFanPwm;
        }
      }
    } else if (tempDeficit >= TEMP_THRESHOLD_LOW) {
      fanPwm = FAN_PWM_MIN;
    }
  }
  
  return fanPwm;
}

int calculateFanSpeedForDisplay(const SystemState& state) {
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

bool calculateVaporizerState(const SystemState& state, const VaporizerState& vaporizerState) {
  if (!state.sensorReadSuccess) {
    return vaporizerState.isOn;
  }
  
  float humDiff = state.humidity - state.humTarget;
  float humDeficit = state.humTarget - state.humidity;
  
  if (humDeficit > 2.0f) {
    return true;
  } else if (humDiff > 2.0f) {
    return false;
  } else {
    return vaporizerState.isOn;
  }
}

FanPwmState updateFanPwm(int fanPwmValue, const FanPwmState& pwmState) {
  FanPwmState newState = pwmState;
  unsigned long now = millis();
  unsigned long cycleTime = now - pwmState.lastCycleStart;

  if (cycleTime >= pwmState.period) {
    newState.lastCycleStart = now;
    cycleTime = 0;
  }

  // Implement kick-start logic
  int effectivePwm = fanPwmValue;
  bool shouldBeOn = (fanPwmValue > 0);
  
  if (shouldBeOn && !pwmState.isOn) {
    // Fan is starting - use kick-start power
    newState.lastStartTime = now;
    effectivePwm = FAN_PWM_START;
  } else if (shouldBeOn && pwmState.isOn) {
    // Fan is running - check if kick-start period is over
    unsigned long timeSinceStart = now - pwmState.lastStartTime;
    if (timeSinceStart < FAN_KICK_START_DURATION) {
      // Still in kick-start period
      effectivePwm = FAN_PWM_START;
    } else {
      // Kick-start period over, use normal PWM
      effectivePwm = fanPwmValue;
    }
  }

  unsigned long onTime = (effectivePwm * pwmState.period) / 255;
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

void applyVaporizerOutput(bool isOn) {
  digitalWrite(VAPORIZER_PIN, isOn ? HIGH : LOW);
} 
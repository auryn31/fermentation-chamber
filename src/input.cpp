#include <Arduino.h>
#include "AiEsp32RotaryEncoder.h"
#include "input.h"
#include "config.h"
#include "persistence.h"

// Global encoder instance
extern AiEsp32RotaryEncoder rotaryEncoder;

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

SystemState processEncoder(const SystemState& state) {
  SystemState newState = state;
  
  if (rotaryEncoder.encoderChanged()) {
    int currentValue = rotaryEncoder.readEncoder();
    
    if (state.menuIndex == 0) {
      // In temperature menu
      newState.tempTarget = currentValue;
      
      // Save temperature target to preferences
      saveTargetTemperature(newState.tempTarget);
    } else if (state.menuIndex == 1) {
      // In humidity menu
      newState.humTarget = currentValue;
      
      // Save humidity target to preferences
      saveTargetHumidity(newState.humTarget);
    } else if (state.menuIndex == 2) {
      // In timer menu - adjust in 5-minute steps
      newState.timerSeconds = currentValue * TIMER_STEP;
      newState.timerOriginalSeconds = newState.timerSeconds;
      // If timer is running and we change the value, restart it
      if (newState.timerRunning) {
        newState.timerStartTime = millis();
      }
    }
    
    newState.lastEncoderValue = currentValue;
  }
  
  return newState;
}

SystemState processButton(const SystemState& state) {
  SystemState newState = state;
  
  if (rotaryEncoder.isEncoderButtonClicked()) {
    unsigned long currentTime = millis();
    
    if (currentTime - state.lastButtonPress > BUTTON_DEBOUNCE_TIME) {
      // For now, just implement short press (menu change)
      // Long press can be added later with additional button state tracking
      newState.menuIndex = (state.menuIndex + 1) % 3;
      
      // Update encoder boundaries and value based on new menu selection
      if (newState.menuIndex == 0) {
        rotaryEncoder.setBoundaries(TEMP_MIN, TEMP_MAX, false);
        rotaryEncoder.setEncoderValue(newState.tempTarget);
      } else if (newState.menuIndex == 1) {
        rotaryEncoder.setBoundaries(HUM_MIN, HUM_MAX, false);
        rotaryEncoder.setEncoderValue(newState.humTarget);
      } else {
        rotaryEncoder.setBoundaries(TIMER_MIN, TIMER_MAX / TIMER_STEP, false);
        rotaryEncoder.setEncoderValue(newState.timerSeconds / TIMER_STEP);
        
        // Auto-start timer when entering timer menu if timer > 0 and not running
        if (newState.timerSeconds > 0 && !newState.timerRunning) {
          newState.timerRunning = true;
          newState.timerOriginalSeconds = newState.timerSeconds;
          newState.timerStartTime = currentTime;
        }
      }
      
      newState.lastButtonPress = currentTime;
    }
  }
  
  return newState;
}

SystemState clampValues(const SystemState& state) {
  SystemState newState = state;
  
  // Pure function that clamps values to allowed ranges
  newState.tempTarget = max(TEMP_MIN, min(newState.tempTarget, TEMP_MAX));
  newState.humTarget = max(HUM_MIN, min(newState.humTarget, HUM_MAX));
  newState.timerSeconds = max((unsigned long)TIMER_MIN, min(newState.timerSeconds, (unsigned long)TIMER_MAX));
  
  return newState;
} 
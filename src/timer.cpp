#include <Arduino.h>
#include "timer.h"

SystemState updateTimer(const SystemState& state) {
  SystemState newState = state;
  
  if (newState.timerRunning && newState.timerOriginalSeconds > 0) {
    unsigned long currentTime = millis();
    unsigned long elapsedSeconds = (currentTime - newState.timerStartTime) / 1000;
    
    if (elapsedSeconds >= newState.timerOriginalSeconds) {
      newState.timerSeconds = 0;
      newState.timerRunning = false;
    } else {
      newState.timerSeconds = newState.timerOriginalSeconds - elapsedSeconds;
    }
  } else if (newState.timerRunning && newState.timerSeconds == 0) {
    newState.timerRunning = false;
  }
  
  return newState;
} 
#include <Arduino.h>
#include <Preferences.h>
#include "persistence.h"
#include "config.h"

// Global preferences object
Preferences preferences;

// Default values if no stored values exist
#define DEFAULT_TEMP_TARGET 10
#define DEFAULT_HUM_TARGET 50

SystemState loadStoredSettings(const SystemState& state) {
  SystemState newState = state;
  
  preferences.begin("fermentation", true);
  
  newState.tempTarget = preferences.getInt("tempTarget", DEFAULT_TEMP_TARGET);
  newState.humTarget = preferences.getInt("humTarget", DEFAULT_HUM_TARGET);
  
  preferences.end();
  return newState;
}

void saveTargetTemperature(int tempTarget) {
  preferences.begin("fermentation", false);
  preferences.putInt("tempTarget", tempTarget);
  preferences.end();
}

void saveTargetHumidity(int humTarget) {
  preferences.begin("fermentation", false);
  preferences.putInt("humTarget", humTarget);
  preferences.end();
} 
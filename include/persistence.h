#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "types.h"

// Function to initialize preferences and load stored values
SystemState loadStoredSettings(const SystemState& state);

// Function to save individual target temperature
void saveTargetTemperature(int tempTarget);

// Function to save individual target humidity
void saveTargetHumidity(int humTarget);

#endif // PERSISTENCE_H 
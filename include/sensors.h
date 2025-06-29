#ifndef SENSORS_H
#define SENSORS_H

#include "types.h"

// Function declarations for sensor operations
SystemState readSensors(const SystemState& state);
float compensateHumidity(float rawHumidity);

#endif // SENSORS_H 
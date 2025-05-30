#ifndef INPUT_H
#define INPUT_H

#include "types.h"

// Function declarations for input operations
SystemState processEncoder(const SystemState& state);
SystemState processButton(const SystemState& state);
SystemState clampValues(const SystemState& state);
void IRAM_ATTR readEncoderISR();

#endif // INPUT_H 
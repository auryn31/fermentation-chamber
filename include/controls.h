#ifndef CONTROLS_H
#define CONTROLS_H

#include "types.h"

// Function declarations for control operations
int calculateFanSpeed(const SystemState& state, const VaporizerState& vaporizerState);
int calculateFanSpeedForDisplay(const SystemState& state);
int calculateHeaterPower(const SystemState& state);
bool calculateVaporizerState(const SystemState& state, const VaporizerState& vaporizerState);
FanPwmState updateFanPwm(int fanPwmValue, const FanPwmState& pwmState);
HeaterPwmState updateHeaterPwm(int heaterPwmValue, const HeaterPwmState& pwmState);
void applyFanOutput(bool isOn);
void applyHeaterOutput(bool isOn);
void applyVaporizerOutput(bool isOn);

#endif // CONTROLS_H 
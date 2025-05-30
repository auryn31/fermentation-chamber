#include <Arduino.h>
#include <DHT.h>
#include "sensors.h"
#include "config.h"

// Global DHT sensor instance
extern DHT dht;

SystemState readSensors(const SystemState& state) {
  SystemState newState = state;
  
  // Read sensors (unavoidable side effect)
  float rawHumidity = dht.readHumidity();
  float rawTemperature = dht.readTemperature();
  
  // Apply voltage compensation if readings are valid
  if (!isnan(rawHumidity) && !isnan(rawTemperature)) {
    newState.humidity = compensateHumidity(rawHumidity);
    newState.temperature = compensateTemperature(rawTemperature);
    newState.sensorReadSuccess = true;
  } else {
    newState.humidity = rawHumidity;  // Keep NaN for error indication
    newState.temperature = rawTemperature;
    newState.sensorReadSuccess = false;
  }
  
  newState.lastSensorRead = millis();
  
  return newState;
}

float compensateHumidity(float rawHumidity) {
  // DHT11 humidity readings tend to be higher when voltage is lower
  // This compensation is based on empirical testing and DHT11 characteristics
  if (isnan(rawHumidity)) return rawHumidity;
  
  // Voltage ratio compensation
  float voltageRatio = SUPPLY_VOLTAGE / NOMINAL_VOLTAGE;
  
  // Non-linear compensation: lower voltage causes higher humidity readings
  // The compensation factor becomes more aggressive at higher humidity values
  float compensatedHumidity = rawHumidity * HUMIDITY_VOLTAGE_COMPENSATION_FACTOR;
  
  // Additional non-linear correction for high humidity values
  if (rawHumidity > 60.0f) {
    float extraCorrection = (rawHumidity - 60.0f) * 0.01f; // 1% per % above 60%
    compensatedHumidity -= extraCorrection;
  }
  
  // Clamp to valid humidity range
  compensatedHumidity = max(0.0f, min(100.0f, compensatedHumidity));
  
  return compensatedHumidity;
}

float compensateTemperature(float rawTemperature) {
  // DHT11 temperature readings are less affected by voltage but still need compensation
  if (isnan(rawTemperature)) return rawTemperature;
  
  // Simple linear offset compensation for temperature
  float compensatedTemperature = rawTemperature + TEMP_VOLTAGE_COMPENSATION_OFFSET;
  
  // Additional voltage-dependent correction
  float voltageRatio = SUPPLY_VOLTAGE / NOMINAL_VOLTAGE;
  float voltageCorrection = (1.0f - voltageRatio) * 2.0f; // 2°C per 0.1V difference
  compensatedTemperature += voltageCorrection;
  
  return compensatedTemperature;
} 
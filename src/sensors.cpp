#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "sensors.h"
#include "config.h"

// Global BME280 sensor instance
extern Adafruit_BME280 bme;

SystemState readSensors(const SystemState& state) {
  SystemState newState = state;
  
  // Read sensors from BME280
  float rawTemperature = bme.readTemperature();
  float rawHumidity = bme.readHumidity();
  
  // Check if readings are valid
  if (!isnan(rawTemperature) && !isnan(rawHumidity)) {
    newState.temperature = rawTemperature;
    newState.humidity = rawHumidity;
    newState.sensorReadSuccess = true;
  } else {
    newState.humidity = rawHumidity;  // Keep NaN for error indication
    newState.temperature = rawTemperature;
    newState.sensorReadSuccess = false;
  }
  
  newState.lastSensorRead = millis();
  
  return newState;
}
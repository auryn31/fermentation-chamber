#ifndef CONFIG_H
#define CONFIG_H

// Pin definitions
#define ENCODER_CLK 2
#define ENCODER_DT  3
#define ENCODER_SW  10
#define FAN_PIN 5
#define HEATER_PIN 21
#define VAPORIZER_PIN 0

// I2C configuration for BME280
#define BME280_I2C_ADDRESS 0x76  // Default I2C address for BME280

// Control constants
#define FAN_PWM_FREQ_SOFT 10 // Software PWM frequency in Hz
#define FAN_PWM_MIN 1       // Minimum PWM for slow operation
#define FAN_PWM_START 50    // PWM value to start the fan (kick-start)
#define FAN_PWM_MAX 255       // Max PWM
#define FAN_KICK_START_DURATION 1000 // Kick-start duration in milliseconds
#define HEATER_PWM_MIN 0      // No minimum for heater
#define HEATER_PWM_MAX 255    // Max PWM for heater
#define TEMP_THRESHOLD_LOW 1  // Minimum degrees below target to turn on heater

// Range limits
#define TEMP_MIN 0
#define TEMP_MAX 40
#define HUM_MIN 0
#define HUM_MAX 100
#define TIMER_MIN 0
#define TIMER_MAX 999999  // Max timer in seconds (about 11.5 days)
#define TIMER_STEP 300    // 5 minutes in seconds

// Timing constants
#define BUTTON_DEBOUNCE_TIME 50
#define BUTTON_LONG_PRESS_TIME 1000  // Long press threshold in ms
#define ROTARY_ENCODER_STEPS 4
#define SENSOR_READ_INTERVAL 500 // Read sensors every 500ms

#endif // CONFIG_H 
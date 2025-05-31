#ifndef CONFIG_H
#define CONFIG_H

// Pin definitions
#define DHTPIN 7
#define DHTTYPE DHT11
#define ENCODER_CLK 3
#define ENCODER_DT  2
#define ENCODER_SW  10
#define FAN_PIN 5
#define HEATER_PIN 21

// Voltage compensation constants for DHT11 at 2.8V
#define SUPPLY_VOLTAGE 3.0f          // Actual supply voltage
#define NOMINAL_VOLTAGE 3.3f         // DHT11 nominal voltage
#define HUMIDITY_VOLTAGE_COMPENSATION_FACTOR 0.85f  // Empirical factor for humidity correction
#define TEMP_VOLTAGE_COMPENSATION_OFFSET -1.5f      // Temperature offset in °C

// Control constants
#define FAN_PWM_FREQ_SOFT 10 // Software PWM frequency in Hz
#define FAN_PWM_MIN 38       // Minimum PWM that reliably spins the fan (15% of 255)
#define FAN_PWM_MAX 255       // Max PWM
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
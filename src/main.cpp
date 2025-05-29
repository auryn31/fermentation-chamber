#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#include "AiEsp32RotaryEncoder.h"

// Pin definitions
#define DHTPIN 7
#define DHTTYPE DHT11
#define ENCODER_CLK 3
#define ENCODER_DT  2
#define ENCODER_SW  10
#define FAN_PIN 5
#define HEATER_PIN 20

// Constants
#define FAN_PWM_FREQ_SOFT 250 // Software PWM frequency in Hz
#define FAN_PWM_MIN 10       // Minimum PWM that reliably spins the fan
#define FAN_PWM_MAX 255       // Max PWM
#define HEATER_PWM_MIN 0      // No minimum for heater
#define HEATER_PWM_MAX 255    // Max PWM for heater
#define TEMP_THRESHOLD_LOW 3  // Minimum degrees below target to turn on heater
#define TEMP_MIN 0
#define TEMP_MAX 40
#define HUM_MIN 0
#define HUM_MAX 100
#define TIMER_MIN 0
#define TIMER_MAX 999999  // Max timer in seconds (about 11.5 days)
#define TIMER_STEP 300    // 5 minutes in seconds
#define BUTTON_DEBOUNCE_TIME 50
#define BUTTON_LONG_PRESS_TIME 1000  // Long press threshold in ms
#define ROTARY_ENCODER_STEPS 4
#define SENSOR_READ_INTERVAL 500 // Read sensors every 500ms

// Hardware initialization
DHT dht(DHTPIN, DHTTYPE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW, -1, ROTARY_ENCODER_STEPS);

// State structure to hold all system state
struct SystemState {
  int tempTarget;
  int humTarget;
  int menuIndex;
  float humidity;
  float temperature;
  bool sensorReadSuccess;
  unsigned long lastButtonPress;
  unsigned long lastSensorRead;
  int lastEncoderValue;
  unsigned long buttonPressStart;    // When button was first pressed
  unsigned long timerSeconds;        // Timer countdown in seconds (remaining time)
  unsigned long timerOriginalSeconds; // Original timer duration
  unsigned long timerStartTime;      // When timer was started (millis())
  bool timerRunning;                 // Whether timer is actively counting down
};

// Fan PWM state structure
struct FanPwmState {
  unsigned long lastCycleStart;
  unsigned long period;
  bool isOn;
};

// Heater PWM state structure
struct HeaterPwmState {
  unsigned long lastCycleStart;
  unsigned long period;
  bool isOn;
};

// Function prototypes - most return a new state rather than modifying global state
void setupHardware();
SystemState createInitialState();
SystemState readSensors(const SystemState& state);
SystemState processEncoder(const SystemState& state);
SystemState processButton(const SystemState& state);
SystemState clampValues(const SystemState& state);
SystemState updateTimer(const SystemState& state);
int calculateFanSpeed(const SystemState& state);
int calculateHeaterPower(const SystemState& state);
void updateDisplay(const SystemState& state);
FanPwmState updateFanPwm(int fanPwmValue, const FanPwmState& pwmState);
HeaterPwmState updateHeaterPwm(int heaterPwmValue, const HeaterPwmState& pwmState);
void applyFanOutput(bool isOn);
void applyHeaterOutput(bool isOn);
void IRAM_ATTR readEncoderISR();

// Global state that can't be easily made functional due to hardware interactions
SystemState state;
FanPwmState fanState = {0, 255, false};
HeaterPwmState heaterState = {0, 255, false};

void setup() {
  setupHardware();
  state = createInitialState();
  state = readSensors(state);
}

void loop() {
  // Read sensors periodically based on time
  if (millis() - state.lastSensorRead >= SENSOR_READ_INTERVAL) {
    state = readSensors(state);
  }

  // Process inputs and update state in a functional way
  state = processEncoder(state);
  state = processButton(state);
  state = clampValues(state);
  state = updateTimer(state);
  
  // Calculate outputs based on state
  int fanPwm = calculateFanSpeed(state);
  int heaterPwm = calculateHeaterPower(state);
  updateDisplay(state);
  
  // Debug output for control values
  static unsigned long lastDebugOutput = 0;
  if (millis() - lastDebugOutput >= 1000) { // Debug output every second
    Serial.print("Fan PWM: "); Serial.print(fanPwm);
    Serial.print(", Heater PWM: "); Serial.println(heaterPwm);
    lastDebugOutput = millis();
  }
  
  // Update fan and heater state and apply to hardware
  fanState = updateFanPwm(fanPwm, fanState);
  heaterState = updateHeaterPwm(heaterPwm, heaterState);
  applyFanOutput(fanState.isOn);
  applyHeaterOutput(heaterState.isOn);
  
  delay(5); // Small delay to help with debouncing
}

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

void setupHardware() {
  Serial.begin(9600);
  delay(300);
  Serial.println(F("Serial started."));
  
  Wire.begin(8, 9);
  u8g2.begin();
  dht.begin();

  // Initialize rotary encoder
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  bool circleValues = false;
  rotaryEncoder.setBoundaries(TEMP_MIN, TEMP_MAX, circleValues);
  rotaryEncoder.setAcceleration(50);
  
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
}

SystemState createInitialState() {
  SystemState newState = {
    .tempTarget = 10,
    .humTarget = 50,
    .menuIndex = 0,
    .humidity = 0,
    .temperature = 0,
    .sensorReadSuccess = false,
    .lastButtonPress = 0,
    .lastSensorRead = 0,
    .lastEncoderValue = 10,
    .buttonPressStart = 0,
    .timerSeconds = 0,
    .timerOriginalSeconds = 0,
    .timerStartTime = 0,
    .timerRunning = false
  };
  
  rotaryEncoder.setEncoderValue(newState.tempTarget);
  
  return newState;
}

SystemState readSensors(const SystemState& state) {
  SystemState newState = state;
  
  // Read sensors (unavoidable side effect)
  newState.humidity = dht.readHumidity();
  newState.temperature = dht.readTemperature();
  newState.sensorReadSuccess = !isnan(newState.humidity) && !isnan(newState.temperature);
  newState.lastSensorRead = millis();
  
  // Debug output (side effect isolated to this function)
  Serial.print("Temp: "); Serial.print(newState.temperature);
  Serial.print("°C, Hum: "); Serial.print(newState.humidity);
  Serial.print("%, Target T: "); Serial.print(newState.tempTarget);
  Serial.print("°C, H: "); Serial.print(newState.humTarget);
  Serial.println("%");
  
  return newState;
}

SystemState processEncoder(const SystemState& state) {
  SystemState newState = state;
  
  if (rotaryEncoder.encoderChanged()) {
    int currentValue = rotaryEncoder.readEncoder();
    
    if (state.menuIndex == 0) {
      // In temperature menu
      newState.tempTarget = currentValue;
      Serial.print("Temperature target: ");
      Serial.println(newState.tempTarget);
    } else if (state.menuIndex == 1) {
      // In humidity menu
      newState.humTarget = currentValue;
      Serial.print("Humidity target: ");
      Serial.println(newState.humTarget);
    } else if (state.menuIndex == 2) {
      // In timer menu - adjust in 5-minute steps
      newState.timerSeconds = currentValue * TIMER_STEP;
      newState.timerOriginalSeconds = newState.timerSeconds;
      // If timer is running and we change the value, restart it
      if (newState.timerRunning) {
        newState.timerStartTime = millis();
      }
      Serial.print("Timer set to: ");
      Serial.println(newState.timerSeconds);
    }
    
    newState.lastEncoderValue = currentValue;
  }
  
  return newState;
}

SystemState processButton(const SystemState& state) {
  SystemState newState = state;
  
  if (rotaryEncoder.isEncoderButtonClicked()) {
    unsigned long currentTime = millis();
    
    if (currentTime - state.lastButtonPress > BUTTON_DEBOUNCE_TIME) {
      // For now, just implement short press (menu change)
      // Long press can be added later with additional button state tracking
      newState.menuIndex = (state.menuIndex + 1) % 3;
      
      // Update encoder boundaries and value based on new menu selection
      if (newState.menuIndex == 0) {
        rotaryEncoder.setBoundaries(TEMP_MIN, TEMP_MAX, false);
        rotaryEncoder.setEncoderValue(newState.tempTarget);
      } else if (newState.menuIndex == 1) {
        rotaryEncoder.setBoundaries(HUM_MIN, HUM_MAX, false);
        rotaryEncoder.setEncoderValue(newState.humTarget);
      } else {
        rotaryEncoder.setBoundaries(TIMER_MIN, TIMER_MAX / TIMER_STEP, false);
        rotaryEncoder.setEncoderValue(newState.timerSeconds / TIMER_STEP);
        
        // Auto-start timer when entering timer menu if timer > 0 and not running
        if (newState.timerSeconds > 0 && !newState.timerRunning) {
          newState.timerRunning = true;
          newState.timerOriginalSeconds = newState.timerSeconds;
          newState.timerStartTime = currentTime;
          Serial.println("Timer auto-started");
        }
      }
      
      Serial.print("Menu changed to: ");
      if (newState.menuIndex == 0) Serial.println("Temperature");
      else if (newState.menuIndex == 1) Serial.println("Humidity");
      else Serial.println("Timer");
      
      newState.lastButtonPress = currentTime;
    }
  }
  
  return newState;
}

SystemState clampValues(const SystemState& state) {
  SystemState newState = state;
  
  // Pure function that clamps values to allowed ranges
  newState.tempTarget = max(TEMP_MIN, min(newState.tempTarget, TEMP_MAX));
  newState.humTarget = max(HUM_MIN, min(newState.humTarget, HUM_MAX));
  newState.timerSeconds = max((unsigned long)TIMER_MIN, min(newState.timerSeconds, (unsigned long)TIMER_MAX));
  
  return newState;
}

SystemState updateTimer(const SystemState& state) {
  SystemState newState = state;
  
  if (newState.timerRunning && newState.timerOriginalSeconds > 0) {
    unsigned long currentTime = millis();
    unsigned long elapsedSeconds = (currentTime - newState.timerStartTime) / 1000;
    
    if (elapsedSeconds >= newState.timerOriginalSeconds) {
      // Timer finished
      newState.timerSeconds = 0;
      newState.timerRunning = false;
      Serial.println("Timer finished!");
    } else {
      // Calculate remaining time
      newState.timerSeconds = newState.timerOriginalSeconds - elapsedSeconds;
      
      // Debug output every 10 seconds
      static unsigned long lastTimerDebug = 0;
      if (currentTime - lastTimerDebug >= 10000) {
        Serial.print("Timer: ");
        Serial.print(newState.timerSeconds);
        Serial.print(" seconds remaining");
        Serial.println();
        lastTimerDebug = currentTime;
      }
    }
  } else if (newState.timerRunning && newState.timerSeconds == 0) {
    // Stop timer if it reaches 0
    newState.timerRunning = false;
  }
  
  return newState;
}

int calculateFanSpeed(const SystemState& state) {
  // Pure function that calculates fan speed based on state
  int fanPwm = FAN_PWM_MIN;  // Default: minimal speed
  
  if (state.sensorReadSuccess) {
    float tempDiff = state.temperature - state.tempTarget;  // Positive when too hot
    float humDiff = state.humidity - state.humTarget;       // Positive when too humid
    float tempDeficit = state.tempTarget - state.temperature; // Positive when too cold
    
    if (tempDiff > 0) {
      // Too hot: speed up fan linearly, max at +10C
      fanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * min(tempDiff, 10.0f) / 10.0f);
    } else if (humDiff > 0) {
      // Humidity too high: fan should be on
      if (tempDeficit >= TEMP_THRESHOLD_LOW) {
        // Too cold and too humid: minimal fan speed (heater will also be on)
        fanPwm = FAN_PWM_MIN;
      } else {
        // Not too cold but too humid: higher fan speed
        fanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * min(humDiff, 50.0f) / 50.0f);
      }
    } else if (tempDeficit >= TEMP_THRESHOLD_LOW) {
      // Too cold but humidity OK: minimal fan speed (heater will be on)
      fanPwm = FAN_PWM_MIN;
    }
  }
  
  return fanPwm;
}

int calculateHeaterPower(const SystemState& state) {
  // Pure function that calculates heater power based on state
  int heaterPwm = 0;  // Default: off
  
  if (state.sensorReadSuccess) {
    float tempDiff = state.tempTarget - state.temperature;  // Positive when too cold
    float humDiff = state.humidity - state.humTarget;       // Positive when too humid
    
    // Heater should be on if:
    // 1. Temperature is 3+ degrees below target, OR
    // 2. Humidity is too high AND temperature is not above target
    if (tempDiff >= TEMP_THRESHOLD_LOW) {
      // Too cold: turn on heater proportionally
      heaterPwm = HEATER_PWM_MIN + int((HEATER_PWM_MAX - HEATER_PWM_MIN) * min(tempDiff, 10.0f) / 10.0f);
    } else if (humDiff > 0 && tempDiff > 0) {
      // Humidity too high and temperature not above target: turn on heater
      heaterPwm = HEATER_PWM_MAX;
    }
  }
  
  return heaterPwm;
}

void updateDisplay(const SystemState& state) {
  // Pure calculation of what to display, with side effects of actually updating display
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_courB08_tf);

  // Temp menu (line 1)
  if (state.menuIndex == 0) u8g2.drawBox(0, 0, 128, 12);
  u8g2.setCursor(2, 10);
  u8g2.setDrawColor(state.menuIndex == 0 ? 0 : 1);
  u8g2.print("T:");
  u8g2.print(state.tempTarget);
  u8g2.print("C ");
  if (state.sensorReadSuccess) u8g2.print(state.temperature, 1);
  else u8g2.print("--");
  u8g2.setDrawColor(1);

  // Humidity menu (line 2)
  if (state.menuIndex == 1) u8g2.drawBox(0, 12, 128, 12);
  u8g2.setCursor(2, 22);
  u8g2.setDrawColor(state.menuIndex == 1 ? 0 : 1);
  u8g2.print("H:");
  u8g2.print(state.humTarget);
  u8g2.print("% ");
  if (state.sensorReadSuccess) u8g2.print(state.humidity, 1);
  else u8g2.print("--");
  u8g2.setDrawColor(1);

  // Timer display (line 3) - highlight when selected
  if (state.menuIndex == 2) u8g2.drawBox(0, 24, 128, 12);
  u8g2.setCursor(2, 34);
  u8g2.setDrawColor(state.menuIndex == 2 ? 0 : 1);
  unsigned long totalSeconds = state.timerSeconds;
  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  
  if (days < 10) u8g2.print("0");
  u8g2.print(days);
  u8g2.print(" ");
  if (hours < 10) u8g2.print("0");
  u8g2.print(hours);
  u8g2.print(":");
  if (minutes < 10) u8g2.print("0");
  u8g2.print(minutes);
  u8g2.print(":");
  if (seconds < 10) u8g2.print("0");
  u8g2.print(seconds);
  u8g2.setDrawColor(1);

  u8g2.sendBuffer();
}

FanPwmState updateFanPwm(int fanPwmValue, const FanPwmState& pwmState) {
  // Compute new PWM state based on current state and value
  FanPwmState newState = pwmState;
  unsigned long now = millis();
  unsigned long cycleTime = now - pwmState.lastCycleStart;

  // Start a new PWM period if needed
  if (cycleTime >= pwmState.period) {
    newState.lastCycleStart = now;
    cycleTime = 0;
  }

  unsigned long onTime = (fanPwmValue * pwmState.period) / 255;
  newState.isOn = (cycleTime < onTime);
  
  #ifdef DEBUG_PWM
  Serial.print("PWM: "); Serial.print(fanPwmValue);
  Serial.print(" On: "); Serial.println(newState.isOn);
  #endif
  
  return newState;
}

HeaterPwmState updateHeaterPwm(int heaterPwmValue, const HeaterPwmState& pwmState) {
  // Compute new PWM state based on current state and value
  HeaterPwmState newState = pwmState;
  unsigned long now = millis();
  unsigned long cycleTime = now - pwmState.lastCycleStart;

  // Start a new PWM period if needed
  if (cycleTime >= pwmState.period) {
    newState.lastCycleStart = now;
    cycleTime = 0;
  }

  unsigned long onTime = (heaterPwmValue * pwmState.period) / 255;
  newState.isOn = (cycleTime < onTime);
  
  #ifdef DEBUG_PWM
  Serial.print("PWM: "); Serial.print(heaterPwmValue);
  Serial.print(" On: "); Serial.println(newState.isOn);
  #endif
  
  return newState;
}

void applyFanOutput(bool isOn) {
  // Hardware output side effect isolated to this function
  digitalWrite(FAN_PIN, isOn ? HIGH : LOW);
}

void applyHeaterOutput(bool isOn) {
  // Hardware output side effect isolated to this function
  digitalWrite(HEATER_PIN, isOn ? HIGH : LOW);
}
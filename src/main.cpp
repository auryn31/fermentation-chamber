#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>

// Pin definitions
#define DHTPIN 7
#define DHTTYPE DHT11
#define ENCODER_CLK 2
#define ENCODER_DT  3
#define ENCODER_SW  10
#define FAN_PIN 5

// Constants
#define FAN_PWM_FREQ_SOFT 250 // Software PWM frequency in Hz
#define FAN_PWM_MIN 100       // Minimum PWM that reliably spins the fan
#define FAN_PWM_MAX 255       // Max PWM
#define SENSOR_READ_INTERVAL 500 // Read sensors every 500ms
#define TEMP_MIN 0
#define TEMP_MAX 40
#define HUM_MIN 0
#define HUM_MAX 100
#define BUTTON_DEBOUNCE_TIME 50

// Hardware initialization
DHT dht(DHTPIN, DHTTYPE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// State structure to hold all system state
struct SystemState {
  int tempTarget;
  int humTarget;
  int menuIndex;
  float humidity;
  float temperature;
  bool sensorReadSuccess;
  int lastStateCLK;
  unsigned long lastButtonPress;
  unsigned long lastSensorRead;
};

// Fan PWM state structure
struct FanPwmState {
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
int calculateFanSpeed(const SystemState& state);
void updateDisplay(const SystemState& state);
FanPwmState updateFanPwm(int fanPwmValue, const FanPwmState& pwmState);
void applyFanOutput(bool isOn);

// Global state that can't be easily made functional due to hardware interactions
SystemState state;
FanPwmState fanState = {0, 255, false};

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
  
  // Calculate outputs based on state
  int fanPwm = calculateFanSpeed(state);
  updateDisplay(state);
  
  // Update fan state and apply to hardware
  fanState = updateFanPwm(fanPwm, fanState);
  applyFanOutput(fanState.isOn);
  
  delay(5); // Small delay to help with debouncing
}

void setupHardware() {
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("Serial started."));
  
  Wire.begin(8, 9);
  u8g2.begin();
  dht.begin();

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(FAN_PIN, OUTPUT);
}

SystemState createInitialState() {
  SystemState newState = {
    .tempTarget = 10,
    .humTarget = 50,
    .menuIndex = 0,
    .humidity = 0,
    .temperature = 0,
    .sensorReadSuccess = false,
    .lastStateCLK = digitalRead(ENCODER_CLK),
    .lastButtonPress = 0,
    .lastSensorRead = 0
  };
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
  Serial.print("°C, H: "); Serial.println(newState.humTarget);
  
  return newState;
}

SystemState processEncoder(const SystemState& state) {
  SystemState newState = state;
  
  // Read hardware (unavoidable side effect)
  int currentStateCLK = digitalRead(ENCODER_CLK);

  // Pure computation based on inputs
  if (currentStateCLK != state.lastStateCLK && currentStateCLK == 1) {
    int dtState = digitalRead(ENCODER_DT); // Another unavoidable side effect
    
    #ifdef DEBUG_ENCODER
    Serial.print("Encoder: ");
    Serial.println(dtState != state.lastStateCLK ? "CCW" : "CW");
    #endif

    if (dtState != state.lastStateCLK) {
      // Counter-clockwise
      if (state.menuIndex == 0) newState.tempTarget--;
      else if (state.menuIndex == 1) newState.humTarget--;
    } else {
      // Clockwise
      if (state.menuIndex == 0) newState.tempTarget++;
      else if (state.menuIndex == 1) newState.humTarget++;
    }
  }
  
  newState.lastStateCLK = currentStateCLK;
  return newState;
}

SystemState processButton(const SystemState& state) {
  SystemState newState = state;
  
  // Read button state (unavoidable side effect)
  int buttonState = digitalRead(ENCODER_SW);
  unsigned long currentTime = millis();
  
  // Pure computation based on inputs
  if (buttonState == LOW) {
    if (currentTime - state.lastButtonPress > BUTTON_DEBOUNCE_TIME) {
      newState.menuIndex = (state.menuIndex + 1) % 2; // Toggle between 0 and 1
    }
    newState.lastButtonPress = currentTime;
  }
  
  return newState;
}

SystemState clampValues(const SystemState& state) {
  SystemState newState = state;
  
  // Pure function that clamps values to allowed ranges
  newState.tempTarget = max(TEMP_MIN, min(newState.tempTarget, TEMP_MAX));
  newState.humTarget = max(HUM_MIN, min(newState.humTarget, HUM_MAX));
  
  return newState;
}

int calculateFanSpeed(const SystemState& state) {
  // Pure function that calculates fan speed based on state
  int fanPwm = FAN_PWM_MIN;  // Default: slow speed
  
  if (state.sensorReadSuccess) {
    float diff = state.temperature - state.tempTarget;
    if (diff > 0) {
      // Too hot: speed up linearly, max at +10C
      fanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * min(diff, 10.0f) / 10.0f);
    }
  }
  
  return fanPwm;
}

void updateDisplay(const SystemState& state) {
  // Pure calculation of what to display, with side effects of actually updating display
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_courB08_tf);

  // Temp menu
  if (state.menuIndex == 0) u8g2.drawBox(0, 10, 128, 20);
  u8g2.setCursor(2, 24);
  u8g2.setDrawColor(state.menuIndex == 0 ? 0 : 1);
  u8g2.print("Temp T: ");
  u8g2.print(state.tempTarget);
  u8g2.print("C G: ");
  if (state.sensorReadSuccess) u8g2.print(state.temperature, 1);
  else u8g2.print("--");
  u8g2.setDrawColor(1);

  // Humidity menu
  if (state.menuIndex == 1) u8g2.drawBox(0, 34, 128, 20);
  u8g2.setCursor(2, 48);
  u8g2.setDrawColor(state.menuIndex == 1 ? 0 : 1);
  u8g2.print("Hum T: ");
  u8g2.print(state.humTarget);
  u8g2.print("% G: ");
  if (state.sensorReadSuccess) u8g2.print(state.humidity, 1);
  else u8g2.print("--");
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

void applyFanOutput(bool isOn) {
  // Hardware output side effect isolated to this function
  digitalWrite(FAN_PIN, isOn ? HIGH : LOW);
}
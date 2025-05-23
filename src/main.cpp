#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Rotary encoder pins
#define ENCODER_CLK 2
#define ENCODER_DT  3
#define ENCODER_SW  10

// --- Fan PWM setup ---
#define FAN_PIN 5           // PWM-capable pin for fan
#define FAN_PWM_FREQ_SOFT 250 // Software PWM frequency in Hz

int tempTarget = 10;
int humTarget = 50;
int menuIndex = 0; // 0: temp, 1: hum

// fan helpers

static unsigned long lastCycleStart = 0;
static unsigned long period = 255;
static bool isOn = false;


int lastClk = HIGH;
int lastDt = HIGH;
bool lastButton = HIGH;

// Function prototype
void handleFanSoftwarePwm(int fanPwmValue);

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("Serial started."));
  Wire.begin(8, 9);
  u8g2.begin();
  dht.begin();

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(FAN_PIN, OUTPUT); // Add this
}

void loop() {
  int clkState = digitalRead(ENCODER_CLK);
  int dtState = digitalRead(ENCODER_DT);
  int buttonState = digitalRead(ENCODER_SW);

  // Only act on falling edge of CLK
  if (lastClk == HIGH && clkState == LOW) {
    if (dtState == HIGH) {
      // Clockwise
      if (menuIndex == 0) tempTarget++;
      else if (menuIndex == 1) humTarget++;
    } else {
      // Counterclockwise
      if (menuIndex == 0) tempTarget--;
      else if (menuIndex == 1) humTarget--;
    }
  }
  lastClk = clkState;

  // Detect button press (menu navigation)
  if (buttonState == LOW && lastButton == HIGH) {
    menuIndex = (menuIndex + 1) % 2; // Toggle between 0 and 1
    delay(200); // Debounce
  }
  lastButton = buttonState;

  // Clamp values
  if (tempTarget < 0) tempTarget = 0;
  if (tempTarget > 40) tempTarget = 40;
  if (humTarget < 0) humTarget = 0;
  if (humTarget > 100) humTarget = 100;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // --- Fan PWM control ---
  const int FAN_PWM_MIN = 100; // Minimum PWM that reliably spins the fan
  const int FAN_PWM_MAX = 255; // Max PWM
  int fanPwm = FAN_PWM_MIN;    // Default: slow speed
  if (!isnan(t)) {
    float diff = t - tempTarget;
    if (diff > 0) {
      // Too hot: speed up linearly, max at +10C
      fanPwm = FAN_PWM_MIN + int((FAN_PWM_MAX - FAN_PWM_MIN) * std::min(diff, 10.0f) / 10.0f);
    }
  }

  // Display
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_courB08_tf);

  // Temp menu
  if (menuIndex == 0) u8g2.drawBox(0, 10, 128, 20);
  u8g2.setCursor(2, 24);
  u8g2.setDrawColor(menuIndex == 0 ? 0 : 1);
  u8g2.print("Temp T: ");
  u8g2.print(tempTarget);
  u8g2.print("C G: ");
  if (!isnan(t)) u8g2.print(t, 1);
  else u8g2.print("--");
  u8g2.setDrawColor(1);

  // Humidity menu
  if (menuIndex == 1) u8g2.drawBox(0, 34, 128, 20);
  u8g2.setCursor(2, 48);
  u8g2.setDrawColor(menuIndex == 1 ? 0 : 1);
  u8g2.print("Hum T: ");
  u8g2.print(humTarget);
  u8g2.print("% G: ");
  if (!isnan(h)) u8g2.print(h, 1);
  else u8g2.print("--");
  u8g2.setDrawColor(1);

  u8g2.sendBuffer();

  handleFanSoftwarePwm(fanPwm); // Pass fanPwm as argument
}

void handleFanSoftwarePwm(int fanPwmValue) {
  unsigned long now = millis();
  unsigned long cycleTime = now - lastCycleStart;

  // Start a new PWM period if needed
  if (cycleTime >= period) {
    lastCycleStart = now;
    cycleTime = 0;
  }

  unsigned long onTime = (fanPwmValue * period) / 255;
  if (cycleTime < onTime) {
    isOn = true;
    digitalWrite(FAN_PIN, HIGH); // Turn on fan
  } else {
    isOn = false;
    digitalWrite(FAN_PIN, LOW); // Turn off fan
  }

  // Set fan state based on duty cycle

  // Debug output
  Serial.print("fanPwmValue: "); Serial.print(fanPwmValue);
  Serial.print(" onTime: "); Serial.print(onTime);
  Serial.print(" cycleTime: "); Serial.print(cycleTime);
  Serial.print(" isOn: "); Serial.println(isOn);
}
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

int tempTarget = 20;
int humTarget = 50;
int menuIndex = 0; // 0: temp, 1: hum

int lastClk = HIGH;
int lastDt = HIGH; // Add this line
bool lastButton = HIGH;

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

}
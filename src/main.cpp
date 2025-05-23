#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h> // Add this line

#define DHTPIN 7         // Set to the GPIO pin you connected DHT11 to
#define DHTTYPE DHT11    // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// U8G2_SH1106_128X64_NONAME_F_HW_I2C: Full buffer, hardware I2C, no reset pin
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("Serial started."));

  // Initialize I2C with explicit pins for ESP32-C3
  Wire.begin(8, 9); // SDA = GPIO8, SCL = GPIO9

  // Initialize display
  u8g2.begin();

  // Initialize DHT sensor
  dht.begin();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tf);
    u8g2.setCursor(0, 24);
    u8g2.print("DHT Error!");
    u8g2.sendBuffer();
    delay(2000);
    return;
  }

  // Print to Serial
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C"));

  // Print to OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tf);
  u8g2.setCursor(0, 24);
  u8g2.print("Temp: ");
  u8g2.print(t, 1);
  u8g2.print(" C");
  u8g2.setCursor(0, 48);
  u8g2.print("Hum:  ");
  u8g2.print(h, 1);
  u8g2.print(" %");
  u8g2.sendBuffer();

  delay(2000); // Wait 2 seconds between updates
}
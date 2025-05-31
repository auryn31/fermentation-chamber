#include <Arduino.h>
#include <U8g2lib.h>
#include "display.h"
#include "controls.h"
#include "config.h"

// Global display instance
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

void updateDisplay(const SystemState& state) {
  // Pure calculation of what to display, with side effects of actually updating display
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_courB12_tf);  // Larger font

  // Temp menu (line 1) - increased spacing
  if (state.menuIndex == 0) u8g2.drawBox(0, 0, 128, 18);
  u8g2.setCursor(2, 15);
  u8g2.setDrawColor(state.menuIndex == 0 ? 0 : 1);
  u8g2.print(state.tempTarget);
  u8g2.print(" / ");
  if (state.sensorReadSuccess) u8g2.print(state.temperature, 1);
  else u8g2.print("--");
  u8g2.print("C");
  u8g2.setDrawColor(1);

  // Humidity menu (line 2) - increased spacing
  if (state.menuIndex == 1) u8g2.drawBox(0, 18, 128, 18);
  u8g2.setCursor(2, 33);
  u8g2.setDrawColor(state.menuIndex == 1 ? 0 : 1);
  u8g2.print(state.humTarget);
  u8g2.print(" / ");
  if (state.sensorReadSuccess) u8g2.print(state.humidity, 1);
  else u8g2.print("--");
  u8g2.print("%");
  u8g2.setDrawColor(1);

  // Timer display (line 3) - highlight when selected, increased spacing
  if (state.menuIndex == 2) u8g2.drawBox(0, 36, 128, 18);
  u8g2.setCursor(2, 51);
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

  // Status indicators (bottom line) - Fan and Heater status
  u8g2.setFont(u8g2_font_6x10_tf);  // Smaller font for status
  u8g2.setCursor(2, 63);
  
  // Calculate current fan and heater values for display
  int fanPwm = calculateFanSpeed(state);
  int heaterPwm = calculateHeaterPower(state);
  
  // Fan indicator
  u8g2.print("F:");
  if (fanPwm > FAN_PWM_MIN) {
    int fanPercent = map(fanPwm, FAN_PWM_MIN, FAN_PWM_MAX, 0, 100);
    u8g2.print(fanPercent);
    u8g2.print("%");
  } else {
    u8g2.print("SLOW");
  }
  
  // Heater indicator
  u8g2.setCursor(65, 63);
  u8g2.print("H:");
  if (heaterPwm > 0) {
    int heaterPercent = map(heaterPwm, 0, 255, 0, 100);  // HEATER_PWM_MIN, HEATER_PWM_MAX
    u8g2.print(heaterPercent);
    u8g2.print("%");
  } else {
    u8g2.print("OFF");
  }

  u8g2.sendBuffer();
} 
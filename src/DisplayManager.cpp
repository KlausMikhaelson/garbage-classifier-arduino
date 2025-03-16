#include "DisplayManager.h"
#include <Arduino.h>

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

namespace DisplayManager {

void initDisplay() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  // Create custom characters, etc.
  byte smileyChar[8] = {
    B00000, B10001, B00000, B00000, B10001, B01110, B00000, B00000
  };
  lcd.createChar(0, smileyChar);
}

void displayWelcome() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Garbage Sorter");
  lcd.setCursor(0, 1);
  lcd.print("Ready for items");
}

void displayThankYou(const String &classification) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Thank You! ");
  lcd.write(byte(0));
  lcd.setCursor(0, 1);
  lcd.print("Type: " + classification);
}

void showWiFiConnected(const String &ip) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(ip);
  delay(2000);
}

} // namespace DisplayManager

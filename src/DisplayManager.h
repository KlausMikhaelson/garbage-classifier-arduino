#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <LiquidCrystal_I2C.h>
#include "Config.h"

namespace DisplayManager {
  extern LiquidCrystal_I2C lcd;
  void initDisplay();
  void displayWelcome();
  void displayThankYou(const String &classification);
  void showWiFiConnected(const String &ip);
}

#endif // DISPLAYMANAGER_H

#include "WiFiManager.h"
#include <Arduino.h>
#include <Wire.h>
#include "DisplayManager.h"
#include "SoundManager.h"

namespace WiFiManager {

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  // Display connection progress (could use DisplayManager functions)
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    dots++;
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  SoundManager::playSuccessSound();
  
  // Optionally, update the display
  DisplayManager::showWiFiConnected(WiFi.localIP().toString());
}
  
} // namespace WiFiManager

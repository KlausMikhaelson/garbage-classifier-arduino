#include <Arduino.h>
#include "Config.h"
#include "WiFiManager.h"
#include "DisplayManager.h"
#include "SoundManager.h"
#include "SensorManager.h"
#include "ServoManager.h"
#include "ServerComm.h"
#include <Wire.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  Wire.begin();
  DisplayManager::initDisplay();
  DisplayManager::displayWelcome();
  
  SoundManager::playStartupSound();
  
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module error!");
    SoundManager::playErrorSound();
    while (true);
  }
  
  WiFiManager::connectToWiFi();
  
  ServoManager::attachServos();
  
  // Test server connection at startup
  String testResult = ServerComm::triggerCaptureEvent();
  if (testResult != "") {
    Serial.println("Server test OK!");
  } else {
    Serial.println("Server test failed!");
  }
  
  DisplayManager::displayWelcome();
}

void loop() {
  long distance = SensorManager::measureDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  if (distance > 0 && distance < THRESHOLD_DISTANCE) {
    Serial.println("Object detected. Triggering capture event...");
    String classification = ServerComm::triggerCaptureEvent();
    int angle = 0;
    if (classification == "recycle") {
      angle = POSITION_RECYCLE;
    } else if (classification == "waste") {
      angle = POSITION_WASTE;
    } else if (classification == "mix") {
      angle = POSITION_MIX;
    } else {
      SoundManager::playErrorSound();
      DisplayManager::initDisplay();
      delay(2000);
      DisplayManager::displayWelcome();
      delay(3000);
      return;
    }
    
    ServoManager::setClassificationAngle(angle);
    DisplayManager::displayThankYou(classification);
    delay(1000);
    ServoManager::operateLid();
    delay(2000);
    DisplayManager::displayWelcome();
    
    delay(3000);
  }
  
  delay(500);
}

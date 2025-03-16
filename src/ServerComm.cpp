#include "ServerComm.h"
#include "Config.h"
#include <WiFiS3.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"
#include "SoundManager.h"

static String cleanChunkedResponse(String response) {
  String cleaned = "";
  int i = 0;
  while (i < response.length()) {
    while (i < response.length() && 
           ((response.charAt(i) >= '0' && response.charAt(i) <= '9') || 
            (response.charAt(i) >= 'a' && response.charAt(i) <= 'f') || 
            (response.charAt(i) >= 'A' && response.charAt(i) <= 'F') ||
            response.charAt(i) == '\r' || response.charAt(i) == '\n')) {
      i++;
    }
    while (i < response.length() && 
           !((response.charAt(i) >= '0' && response.charAt(i) <= '9') && 
             (i + 1 < response.length() && response.charAt(i + 1) == '\r'))) {
      cleaned += response.charAt(i);
      i++;
    }
  }
  int jsonStart = cleaned.indexOf('{');
  if (jsonStart >= 0) {
    cleaned = cleaned.substring(jsonStart);
  }
  int jsonEnd = cleaned.lastIndexOf('}');
  if (jsonEnd >= 0 && jsonEnd < cleaned.length() - 1) {
    cleaned = cleaned.substring(0, jsonEnd + 1);
  }
  return cleaned;
}

namespace ServerComm {

String triggerCaptureEvent() {
  WiFiClient client;
  String response = "";
  
  SoundManager::playAlertSound();
  DisplayManager::initDisplay();  // or a specific update function
  DisplayManager::displayWelcome();
  
  Serial.print("Connecting to ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(httpPort);
  
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection to server failed");
    SoundManager::playErrorSound();
    DisplayManager::initDisplay();
    return "";
  }
  
  client.print("GET /api/trigger-capture HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("Connection: close\r\n\r\n");
  
  unsigned long timeout = millis();
  while (!client.available() && (millis() - timeout < 15000)) {
    delay(100);
  }
  
  if (!client.available()) {
    Serial.println("No response received from server.");
    client.stop();
    SoundManager::playErrorSound();
    DisplayManager::initDisplay();
    return "";
  }
  
  String fullResponse = "";
  bool headersEnded = false;
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (!headersEnded) {
      if (line.length() <= 2) {
        headersEnded = true;
      }
    } else {
      fullResponse += line + "\n";
    }
  }
  client.stop();
  
  String cleanedResponse = cleanChunkedResponse(fullResponse);
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, cleanedResponse);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    SoundManager::playErrorSound();
    DisplayManager::initDisplay();
    return "";
  }
  
  String classification;
  if (doc.containsKey("data") && doc["data"].containsKey("classification")) {
    classification = doc["data"]["classification"].as<String>();
  }
  else if (doc.containsKey("classification")) {
    classification = doc["classification"].as<String>();
  }
  
  if (classification != "") {
    SoundManager::playSuccessSound();
    return classification;
  }
  
  SoundManager::playErrorSound();
  DisplayManager::initDisplay();
  return "";
}

} // namespace ServerComm

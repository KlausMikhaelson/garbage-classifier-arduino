#include <WiFiS3.h>
#include <Servo.h>
#include <ArduinoJson.h>  // You'll need to install this library

// WiFi credentials
char ssid[] = "Klaus";
char pass[] = "00000000";

// Server host and port (updated to match your server)
const char* host = "172.20.10.2";  // This should match the IP shown in the server console
const int httpPort = 3000;  // Changed from 3001 to 3000 to match your server

Servo classificationServo;  // Renamed to be more descriptive
Servo lidServo;             // New servo for the lid

// Servo pins
const int CLASSIFICATION_SERVO_PIN = 9;
const int LID_SERVO_PIN = 10;       // Pin for the lid servo

// Lid servo positions
const int LID_CLOSED = 0;           // Lid closed position (degrees)
const int LID_OPEN = 90;            // Lid open position (degrees)
const int LID_OPEN_TIME = 2000;     // How long to keep the lid open (milliseconds)

// Ultrasonic sensor pins
const int TRIGGER_PIN = 7;
const int ECHO_PIN = 8;

// Threshold distance in centimeters (object detected)
const long THRESHOLD_DISTANCE = 20; // 20cm threshold

// Function to measure distance using an ultrasonic sensor (in cm)
long measureDistance() {
  // Ensure trigger is LOW
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  // Send a 10us pulse on trigger pin
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Read the echo pin (pulseIn returns time in microseconds)
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout after 30ms
  // Calculate distance in centimeters:
  long distance = duration * 0.034 / 2;
  return distance;
}

// Function to clean chunked encoding from response
String cleanChunkedResponse(String response) {
  // Remove any chunk size indicators (hexadecimal numbers followed by CRLF)
  String cleaned = "";
  int i = 0;
  
  while (i < response.length()) {
    // Skip chunk size and CRLF
    while (i < response.length() && 
           ((response.charAt(i) >= '0' && response.charAt(i) <= '9') || 
            (response.charAt(i) >= 'a' && response.charAt(i) <= 'f') || 
            (response.charAt(i) >= 'A' && response.charAt(i) <= 'F') ||
            response.charAt(i) == '\r' || response.charAt(i) == '\n')) {
      i++;
    }
    
    // Add content until next chunk size or end
    while (i < response.length() && 
           !((response.charAt(i) >= '0' && response.charAt(i) <= '9') && 
             (i + 1 < response.length() && response.charAt(i + 1) == '\r'))) {
      cleaned += response.charAt(i);
      i++;
    }
  }
  
  // Find the start of the JSON object (first '{')
  int jsonStart = cleaned.indexOf('{');
  if (jsonStart >= 0) {
    cleaned = cleaned.substring(jsonStart);
  }
  
  // Find the end of the JSON object (last '}')
  int jsonEnd = cleaned.lastIndexOf('}');
  if (jsonEnd >= 0 && jsonEnd < cleaned.length() - 1) {
    cleaned = cleaned.substring(0, jsonEnd + 1);
  }
  
  return cleaned;
}

// Function to trigger the capture event on the server and get classification result
String triggerCaptureEvent() {
  WiFiClient client;
  String response = "";
  
  Serial.print("Connecting to ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(httpPort);
  
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection to server failed");
    return "";
  }
  
  // Send an HTTP GET request to the trigger-capture endpoint
  client.print("GET /api/trigger-capture HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("Connection: close\r\n\r\n");
  
  Serial.println("Request sent, waiting for response...");
  
  // Wait for the response with a timeout
  unsigned long timeout = millis();
  while (!client.available() && (millis() - timeout < 15000)) {  // Increased timeout to 15 seconds
    delay(100);
  }
  
  if (!client.available()) {
    Serial.println("No response received from server.");
    client.stop();
    return "";
  }
  
  Serial.println("Response received!");
  
  // Skip HTTP headers and read the entire response
  bool headersEnded = false;
  String fullResponse = "";
  
  while (client.available()) {
    String line = client.readStringUntil('\n');
    
    if (!headersEnded) {
      if (line.length() <= 2) {  // Empty line or just \r
        headersEnded = true;
      }
    } else {
      fullResponse += line + "\n";
    }
  }
  
  client.stop();
  
  Serial.print("Raw response: ");
  Serial.println(fullResponse);
  
  // Clean the chunked encoding
  String cleanedResponse = cleanChunkedResponse(fullResponse);
  
  Serial.print("Cleaned JSON: ");
  Serial.println(cleanedResponse);
  
  // Parse the JSON response
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, cleanedResponse);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return "";
  }
  
  // Extract the classification from the response
  // First try the format from the trigger-capture endpoint
  if (doc.containsKey("data") && doc["data"].containsKey("classification")) {
    String classification = doc["data"]["classification"].as<String>();
    Serial.print("Classification from JSON: ");
    Serial.println(classification);
    return classification;
  }
  // Fall back to the format from the arduino-test endpoint
  else if (doc.containsKey("classification")) {
    String classification = doc["classification"].as<String>();
    Serial.print("Classification from JSON: ");
    Serial.println(classification);
    return classification;
  }
  
  return "";
}

// Function to operate the lid servo
void operateLid() {
  Serial.println("Opening lid...");
  lidServo.write(LID_OPEN);
  delay(LID_OPEN_TIME);
  Serial.println("Closing lid...");
  lidServo.write(LID_CLOSED);
}

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial Monitor
  
  // Set up ultrasonic sensor pins
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Check for WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }
  
  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Attach the servos to their pins
  classificationServo.attach(CLASSIFICATION_SERVO_PIN);
  lidServo.attach(LID_SERVO_PIN);
  
  // Initialize lid to closed position
  lidServo.write(LID_CLOSED);
  
  // Test the server connection at startup
  Serial.println("Testing server connection and trigger capture...");
  String testResult = triggerCaptureEvent();
  if (testResult != "") {
    Serial.println("Server connection successful!");
    Serial.print("Test classification: ");
    Serial.println(testResult);
  } else {
    Serial.println("Failed to connect to server or trigger capture. Please check your settings.");
  }
}

void loop() {
  // Measure distance from the ultrasonic sensor
  long distance = measureDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  // If an object is detected within threshold, trigger the capture event
  if (distance > 0 && distance < THRESHOLD_DISTANCE) {
    Serial.println("Object detected. Triggering capture event...");
    String classification = triggerCaptureEvent();
    Serial.print("Server classification: ");
    Serial.println(classification);
    
    int angle = 0;
    // Decide servo angle based on the classification result
    if (classification == "recycle") {
      angle = 30;
    } else if (classification == "waste") {
      angle = 60;
    } else if (classification == "mix") {
      angle = 200;
    } else {
      Serial.println("Unknown or empty classification received.");
    }
  
    // Rotate the classification servo if a valid angle is set
    if (angle > 0) {
      classificationServo.write(angle);
      Serial.print("Rotating classification servo to ");
      Serial.print(angle);
      Serial.println(" degrees");
      
      // Wait 1 second after classification servo rotates
      delay(1000);
      
      // Operate the lid
      operateLid();
    }
  
    // Wait a moment after processing to prevent rapid retriggers
    delay(3000);
  }
  
  // Check sensor every 500ms
  delay(500);
}

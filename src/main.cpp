#include <WiFiS3.h>
#include <Servo.h>
#include <ArduinoJson.h>  // You'll need to install this library
#include <Wire.h>         // For I2C communication
#include <LiquidCrystal_I2C.h> // For the I2C LCD display

// WiFi credentials, this is hardcoded for now but in future we can add e-sim or connect it to university wifi
char ssid[] = "Klaus";
char pass[] = "00000000";

// Server host and port (updated to match your server)
const char* host = "172.20.10.2";  // This should match the IP shown in the server console, it's the local IP of my device, used hotspot for it
const int httpPort = 3000;  // port number the server and webapp are running on

Servo classificationServo;  // rotates the bottom servo
Servo lidServo;             // rotates the lid servo

// Servo pins
const int CLASSIFICATION_SERVO_PIN = 9;
const int LID_SERVO_PIN = 10;       // Pin for the lid servo

// Speaker pin
const int SPEAKER_PIN = 6;          // Pin for the speaker/buzzer

// Sound frequencies (in Hz)
const int NOTE_C5 = 523;
const int NOTE_E5 = 659;
const int NOTE_G5 = 784;
const int NOTE_C6 = 1047;

// Classification servo positions (degrees)
const int POSITION_MIX = 0;      // Position for mixed items
const int POSITION_WASTE = 90;     // Position for waste items
const int POSITION_RECYCLE = 135;   // Position for recycle items

// Lid servo positions
const int LID_CLOSED = 100;           // Lid closed position (degrees)
const int LID_OPEN = 0;            // Lid open position (degrees)
const int LID_OPEN_TIME = 2000;     // How long to keep the lid open (milliseconds)

// LCD display settings
const int LCD_I2C_ADDR = 0x27;  // Default I2C address for most LCD displays (might be 0x3F for some)
const int LCD_COLS = 16;        // Most common LCD is 16x2
const int LCD_ROWS = 2;

// Initialize the I2C LCD
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

// Custom character for smiley face
byte smileyChar[8] = {
  B00000,
  B10001,
  B00000,
  B00000,
  B10001,
  B01110,
  B00000,
  B00000
};

// Ultrasonic sensor pins
const int TRIGGER_PIN = 7;
const int ECHO_PIN = 8;

// Threshold distance in centimeters (object detected)
const long THRESHOLD_DISTANCE = 9; // 9cm threshold

// Function to play a success sound
void playSuccessSound() {
  // Play a happy ascending arpeggio
  tone(SPEAKER_PIN, NOTE_C5, 150);
  delay(150);
  tone(SPEAKER_PIN, NOTE_E5, 150);
  delay(150);
  tone(SPEAKER_PIN, NOTE_G5, 150);
  delay(150);
  tone(SPEAKER_PIN, NOTE_C6, 300);
  delay(300);
  noTone(SPEAKER_PIN);
}

// Function to play an error sound
void playErrorSound() {
  // Play a descending tone
  tone(SPEAKER_PIN, NOTE_C6, 200);
  delay(200);
  tone(SPEAKER_PIN, NOTE_E5, 200);
  delay(200);
  tone(SPEAKER_PIN, NOTE_C5, 400);
  delay(400);
  noTone(SPEAKER_PIN);
}

// Function to play an alert sound
void playAlertSound() {
  // Play a short beep
  tone(SPEAKER_PIN, NOTE_G5, 100);
  delay(100);
  noTone(SPEAKER_PIN);
  delay(50);
  tone(SPEAKER_PIN, NOTE_G5, 100);
  delay(100);
  noTone(SPEAKER_PIN);
}

// Function to play a startup sound
void playStartupSound() {
  // Play a startup sequence
  tone(SPEAKER_PIN, NOTE_C5, 100);
  delay(120);
  tone(SPEAKER_PIN, NOTE_E5, 100);
  delay(120);
  tone(SPEAKER_PIN, NOTE_G5, 100);
  delay(120);
  tone(SPEAKER_PIN, NOTE_C6, 300);
  delay(300);
  noTone(SPEAKER_PIN);
}

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
  
  // Play alert sound
  playAlertSound();
  
  // Show "Processing..." on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Processing...");
  lcd.setCursor(0, 1);
  lcd.print("Please wait");
  
  Serial.print("Connecting to ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(httpPort);
  
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection to server failed");
    
    // Play error sound
    playErrorSound();
    
    // Show error on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Server Error");
    lcd.setCursor(0, 1);
    lcd.print("Try again later");
    
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
    
    // Play error sound
    playErrorSound();
    
    // Show error on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Response");
    lcd.setCursor(0, 1);
    lcd.print("Try again later");
    
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
    
    // Play error sound
    playErrorSound();
    
    // Show error on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("JSON Error");
    lcd.setCursor(0, 1);
    lcd.print("Invalid response");
    
    return "";
  }
  
  // Extract the classification from the response
  // First try the format from the trigger-capture endpoint
  if (doc.containsKey("data") && doc["data"].containsKey("classification")) {
    String classification = doc["data"]["classification"].as<String>();
    Serial.print("Classification from JSON: ");
    Serial.println(classification);
    
    // Play success sound
    playSuccessSound();
    
    return classification;
  }
  // Fall back to the format from the arduino-test endpoint
  else if (doc.containsKey("classification")) {
    String classification = doc["classification"].as<String>();
    Serial.print("Classification from JSON: ");
    Serial.println(classification);
    
    // Play success sound
    playSuccessSound();
    
    return classification;
  }
  
  // Play error sound
  playErrorSound();
  
  // Show error on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Classification");
  lcd.setCursor(0, 1);
  lcd.print("Not found");
  
  return "";
}

// Function to operate the lid servo
void operateLid() {
  Serial.println("Opening lid...");
  lidServo.write(LID_OPEN);
  
  // Play a short tone when lid opens
  tone(SPEAKER_PIN, NOTE_E5, 200);
  delay(200);
  noTone(SPEAKER_PIN);
  
  delay(LID_OPEN_TIME - 200); // Subtract the tone duration
  
  Serial.println("Closing lid...");
  lidServo.write(LID_CLOSED);
  
  // Play a short tone when lid closes
  tone(SPEAKER_PIN, NOTE_C5, 200);
  delay(200);
  noTone(SPEAKER_PIN);
}

// Function to display thank you message with smiley
void displayThankYou(String classification) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Thank You! ");
  lcd.write(byte(0)); // Display the smiley character
  
  lcd.setCursor(0, 1);
  lcd.print("Type: ");
  lcd.print(classification);
}

// Function to display welcome message
void displayWelcome() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Garbage Sorter");
  lcd.setCursor(0, 1);
  lcd.print("Ready for items");
}

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial Monitor
  
  // Set up speaker pin
  pinMode(SPEAKER_PIN, OUTPUT);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Create the smiley character
  lcd.createChar(0, smileyChar);
  
  // Display initial message
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  
  // Play startup sound
  playStartupSound();
  
  // Set up ultrasonic sensor pins
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Check for WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    
    // Play error sound
    playErrorSound();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Module");
    lcd.setCursor(0, 1);
    lcd.print("Not Found!");
    
    while (true);
  }
  
  // Connect to WiFi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // Show connection progress on LCD
    lcd.setCursor(dots % 16, 1);
    lcd.print(".");
    dots++;
    
    // Wrap around to beginning of line
    if (dots % 16 == 0) {
      lcd.setCursor(0, 1);
      lcd.print("                "); // Clear the line
    }
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Play success sound
  playSuccessSound();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  
  // Attach the servos to their pins
  classificationServo.attach(CLASSIFICATION_SERVO_PIN);
  lidServo.attach(LID_SERVO_PIN);
  
  // Initialize lid to closed position
  lidServo.write(LID_CLOSED);
  
  // Initialize classification servo to middle position
  classificationServo.write(POSITION_WASTE); // Start at middle position
  
  // Test the server connection at startup
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testing server");
  lcd.setCursor(0, 1);
  lcd.print("connection...");
  
  Serial.println("Testing server connection and trigger capture...");
  String testResult = triggerCaptureEvent();
  if (testResult != "") {
    Serial.println("Server connection successful!");
    Serial.print("Test classification: ");
    Serial.println(testResult);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Server test OK!");
    delay(2000);
  } else {
    Serial.println("Failed to connect to server or trigger capture. Please check your settings.");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Server test");
    lcd.setCursor(0, 1);
    lcd.print("failed!");
    delay(2000);
  }
  
  // Show welcome message when ready
  displayWelcome();
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
      angle = POSITION_RECYCLE;  // Position 1: 45 degrees
      Serial.println("Classification: recycle");
    } else if (classification == "waste") {
      angle = POSITION_WASTE;    // Position 2: 90 degrees (middle)
      Serial.println("Classification: waste");
    } else if (classification == "mix") {
      angle = POSITION_MIX;      // Position 3: 135 degrees
      Serial.println("Classification: mix");
    } else {
      Serial.println("Unknown or empty classification received.");
      
      // Play error sound
      playErrorSound();
      
      // Show error on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Unknown Item");
      lcd.setCursor(0, 1);
      lcd.print("Try again");
      delay(2000);
      displayWelcome();
      delay(3000);
      return;
    }
  
    // Rotate the classification servo if a valid angle is set
    if (angle > 0) {
      classificationServo.write(angle);
      Serial.print("Rotating classification servo to ");
      Serial.print(angle);
      Serial.println(" degrees");
      
      // Display thank you message with classification
      displayThankYou(classification);
      
      // Wait 1 second after classification servo rotates
      delay(1000);
      
      // Operate the lid
      operateLid();
      
      // Wait a bit with the thank you message
      delay(2000);
      
      // Return to welcome screen
      displayWelcome();
    }
  
    // Wait a moment after processing to prevent rapid retriggers
    delay(3000);
  }
  
  // Check sensor every 500ms
  delay(500);
}



// BELOW CODE IS JUST TO TEST SERVO AND IT'S ANGLES, IGNORE THEM!

// #include <Arduino.h>
// #include <Servo.h>

// // Create a servo object
// Servo classificationServo;

// // Servo pin
// const int CLASSIFICATION_SERVO_PIN = 9;  // Same pin as in your main project

// // Servo positions (degrees)
// const int POSITION_1 = 0;   // Recycle position
// const int POSITION_2 = 110;   // Waste position
// const int POSITION_3 = 180;  // Mix position

// // Function prototype
// void sweepServo();

// void setup() {
//   // Initialize serial communication
//   Serial.begin(9600);
//   while (!Serial);  // Wait for Serial Monitor
  
//   // Attach the servo to its pin
//   classificationServo.attach(CLASSIFICATION_SERVO_PIN);
  
//   // Initial position
//   classificationServo.write(90);  // Start at middle position
  
//   Serial.println("Servo Test Program");
//   Serial.println("------------------");
//   Serial.println("Enter a number to move the servo:");
//   Serial.println("1: Move to Position 1 (45 degrees) - Recycle");
//   Serial.println("2: Move to Position 2 (90 degrees) - Waste");
//   Serial.println("3: Move to Position 3 (135 degrees) - Mix");
//   Serial.println("4: Sweep through all positions");
// }

// void loop() {
//   // Check if data is available to read
//   if (Serial.available() > 0) {
//     // Read the incoming byte
//     char input = Serial.read();
    
//     // Process the input
//     switch (input) {
//       case '1':
//         Serial.println("Moving to Position 1 (45 degrees) - Recycle");
//         classificationServo.write(POSITION_1);
//         break;
        
//       case '2':
//         Serial.println("Moving to Position 2 (90 degrees) - Waste");
//         classificationServo.write(POSITION_2);
//         break;
        
//       case '3':
//         Serial.println("Moving to Position 3 (135 degrees) - Mix");
//         classificationServo.write(POSITION_3);
//         break;
        
//       case '4':
//         Serial.println("Sweeping through all positions");
//         sweepServo();
//         break;
        
//       default:
//         // Ignore other characters (like newlines)
//         break;
//     }
//   }
  
//   // Small delay to prevent overwhelming the serial port
//   delay(10);
// }

// // Function to sweep the servo through all positions
// void sweepServo() {
//   // Move to Position 1
//   Serial.println("Moving to Position 1 (45 degrees)");
//   classificationServo.write(POSITION_1);
//   delay(1000);
  
//   // Move to Position 2
//   Serial.println("Moving to Position 2 (90 degrees)");
//   classificationServo.write(POSITION_2);
//   delay(1000);
  
//   // Move to Position 3
//   Serial.println("Moving to Position 3 (135 degrees)");
//   classificationServo.write(POSITION_3);
//   delay(1000);
  
//   // Return to middle position
//   Serial.println("Returning to middle position");
//   classificationServo.write(POSITION_2);
//   delay(1000);
// }

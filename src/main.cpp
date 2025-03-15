#include <WiFiS3.h>
#include <Servo.h>

// WiFi credentials
char ssid[] = "Klaus";
char pass[] = "00000000";

// API host and port
// Replace "localhost" with the actual IP if needed.
const char* host = "172.20.10.2";
const int httpPort = 3001;

Servo myServo;

// Function to perform an HTTP GET request and return the response body
String getAPIResponse() {
  WiFiClient client;
  String response = "";
  
  Serial.print("Connecting to ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(httpPort);
  
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection to API failed");
    return "";
  }
  
  // Send the HTTP GET request
  client.print("GET /garbage HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("Connection: close\r\n\r\n");
  
  // Wait for the response with a timeout
  unsigned long timeout = millis();
  while (!client.available() && (millis() - timeout < 5000)) {
    delay(100);
  }
  
  if (!client.available()) {
    Serial.println("No response received from API.");
    return "";
  }
  
  // Skip HTTP headers
  bool headersEnded = false;
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (!headersEnded) {
      if (line == "\r") {
        headersEnded = true; // empty line indicates end of headers
      }
    } else {
      // Read the first line of the body (assuming the API sends a single-line response)
      response = line;
      break;
    }
  }
  
  response.trim();  // Remove any extra whitespace/newlines
  return response;
}

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial monitor to open

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
  
  // Attach the servo to pin 9
  myServo.attach(9);
}

void loop() {
  String apiResponse = getAPIResponse();
  Serial.print("API Response: ");
  Serial.println(apiResponse);

  int angle = 0;
  
  // Decide servo angle based on API response
  if (apiResponse == "recycle") {
    angle = 30;
  } else if (apiResponse == "waste") {
    angle = 60;
  } else if (apiResponse == "mix") {
    angle = 200;
  } else {
    Serial.println("Unknown or empty response received.");
  }

  // Rotate the servo if a valid angle is set
  if (angle > 0) {
    myServo.write(angle);
    Serial.print("Rotating servo to ");
    Serial.print(angle);
    Serial.println(" degrees");
  }
  
  // Wait before sending another API request
  delay(5000);
}

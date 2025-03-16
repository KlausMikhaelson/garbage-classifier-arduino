#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
const char ssid[] = "Klaus";
const char pass[] = "00000000";

// Server settings
const char* host = "172.20.10.2";
const int httpPort = 3000;

// Servo pins and positions
const int CLASSIFICATION_SERVO_PIN = 9;
const int LID_SERVO_PIN = 10;
const int POSITION_MIX = 0;
const int POSITION_WASTE = 90;
const int POSITION_RECYCLE = 135;
const int LID_CLOSED = 100;
const int LID_OPEN = 0;
const int LID_OPEN_TIME = 2000;

// Speaker pin and sound frequencies
const int SPEAKER_PIN = 6;
const int NOTE_C5 = 523;
const int NOTE_E5 = 659;
const int NOTE_G5 = 784;
const int NOTE_C6 = 1047;

// LCD settings
const int LCD_I2C_ADDR = 0x27;
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

// Ultrasonic sensor pins and threshold
const int TRIGGER_PIN = 7;
const int ECHO_PIN = 8;
const long THRESHOLD_DISTANCE = 1; // in cm

#endif // CONFIG_H
